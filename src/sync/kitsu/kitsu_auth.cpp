/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRestReply>
#include <QTimer>
#include <QUrlQuery>

#include "base/string.hpp"
#include "kitsu.hpp"
#include "sync/kitsu/kitsu_error.hpp"
#include "sync/kitsu/kitsu_utils.hpp"
#include "taiga/accounts.hpp"
#include "taiga/network.hpp"

namespace sync::kitsu {

namespace {

// kitsu.app sits behind Cloudflare, which answers requests without a User-Agent header with a
// 403 challenge page. Unlike API requests made through `api_` (which carry the common headers),
// the token requests are built from scratch, so the headers have to be applied explicitly here.
QNetworkRequest tokenRequest() {
  QNetworkRequest request{QUrl{kTokenUrl}};
  request.setHeaders(taiga::NetworkAccessManager::commonHeaders());
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  return request;
}

}  // namespace

bool Service::retryOnTokenExpiry(QRestReply& reply, std::function<void()> retry) {
  if (!isTokenExpired(reply)) return false;
  refreshAccessToken(std::move(retry));
  return true;
}

void Service::authenticateUser() {
  authenticateUser(0);
}

void Service::authenticateUser(const int attempt) {
  auto request = tokenRequest();

  auto username = taiga::accounts.kitsuEmail();
  if (username.empty()) username = taiga::accounts.kitsuUsername();

  // Resource Owner Password Credentials Grant
  // https://tools.ietf.org/html/rfc6749#section-4.3
  const taiga::NameValuePairs body{
      {"grant_type", "password"},
      {"username", QString::fromStdString(username)},
      {"password", QString::fromStdString(taiga::accounts.kitsuPassword())},
      {"client_id", kClientId},
      {"client_secret", kClientSecret},
  };

  const auto callback = [this, attempt](QRestReply& reply) {
    if (isError(reply)) {
      if (attempt < kMaxRetries && isTransientError(reply)) {
        const auto delay = retryDelay(attempt);
        qWarning() << tagMessage(id(), u"Authentication request failed (HTTP %1), retrying in %2 ms"_s
                                            .arg(reply.httpStatus())
                                            .arg(delay));
        QTimer::singleShot(delay, this, [this, attempt] { authenticateUser(attempt + 1); });
        return;
      }

      handleError(*this, reply);
      emit authenticationCompleted(false);
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse authentication data.");
      emit authenticationCompleted(false);
      return;
    }

    const auto root = json->object();
    const auto accessToken = root["access_token"].toString();
    taiga::accounts.setKitsuAccessToken(accessToken.toStdString());
    taiga::accounts.setKitsuRefreshToken(root["refresh_token"].toString().toStdString());
    api_.setBearerToken(accessToken.toUtf8());

    // Kitsu's token response doesn't include user information, so we need to make an additional
    // request to resolve it.
    resolveUser();
  };

  manager_.post(request, taiga::formUrlEncode(body), this, callback);
}

void Service::refreshAccessToken(std::function<void()> onSuccess, const int attempt) {
  const auto refreshToken = taiga::accounts.kitsuRefreshToken();

  if (refreshToken.empty()) {
    emit errorOccurred("Refresh token is unavailable.");
    emit authenticationCompleted(false);
    return;
  }

  auto request = tokenRequest();

  const taiga::NameValuePairs body{
      {"grant_type", "refresh_token"},
      {"refresh_token", QString::fromStdString(refreshToken)},
      {"client_id", kClientId},
      {"client_secret", kClientSecret},
  };

  const auto callback = [this, onSuccess, attempt](QRestReply& reply) {
    if (isError(reply)) {
      if (attempt < kMaxRetries && isTransientError(reply)) {
        const auto delay = retryDelay(attempt);
        qWarning() << tagMessage(id(), u"Token refresh failed (HTTP %1), retrying in %2 ms"_s
                                            .arg(reply.httpStatus())
                                            .arg(delay));
        QTimer::singleShot(delay, this, [this, onSuccess, attempt] {
          refreshAccessToken(onSuccess, attempt + 1);
        });
        return;
      }

      handleError(*this, reply);
      emit authenticationCompleted(false);
      return;
    }

    const auto json = reply.readJson();
    if (!json) {
      handleError(*this, reply, "Could not parse authentication data.");
      emit authenticationCompleted(false);
      return;
    }

    const auto root = json->object();
    const auto accessToken = root["access_token"].toString();
    taiga::accounts.setKitsuAccessToken(accessToken.toStdString());
    taiga::accounts.setKitsuRefreshToken(root["refresh_token"].toString().toStdString());
    api_.setBearerToken(accessToken.toUtf8());

    if (onSuccess) onSuccess();
  };

  manager_.post(request, taiga::formUrlEncode(body), this, callback);
}

////////////////////////////////////////////////////////////////////////////////

void Service::resolveUser(std::function<void()> onSuccess, const int attempt) {
  const bool authenticated = !taiga::accounts.kitsuAccessToken().empty();

  QUrlQuery query =
      authenticated
          ? QUrlQuery{{u"filter[self]"_s, u"true"_s}}
          : QUrlQuery{{u"filter[slug]"_s, QString::fromStdString(taiga::accounts.kitsuUsername())}};
  query.addQueryItem(u"fields[users]"_s, userFields());

  const auto callback = [this, authenticated, onSuccess, attempt](QRestReply& reply) {
    if (isError(reply)) {
      if (retryOnTokenExpiry(reply, [this, onSuccess] { resolveUser(onSuccess); })) return;

      if (attempt < kMaxRetries && isTransientError(reply)) {
        const auto delay = retryDelay(attempt);
        qWarning() << tagMessage(id(), u"User lookup failed (HTTP %1), retrying in %2 ms"_s
                                            .arg(reply.httpStatus())
                                            .arg(delay));
        QTimer::singleShot(delay, this,
                           [this, onSuccess, attempt] { resolveUser(onSuccess, attempt + 1); });
        return;
      }

      handleError(*this, reply);
      if (authenticated) emit authenticationCompleted(false);
      return;
    }

    const auto json = reply.readJson();
    const auto data = json ? json->object()["data"].toArray() : QJsonArray{};

    if (data.isEmpty()) {
      handleError(*this, reply, "Could not parse user object.");
      if (authenticated) emit authenticationCompleted(false);
      return;
    }

    const auto user = data.first().toObject();
    const auto attributes = user["attributes"].toObject();

    taiga::accounts.setKitsuUserId(user["id"].toString().toStdString());
    taiga::accounts.setKitsuDisplayName(attributes["name"].toString().toStdString());
    taiga::accounts.setKitsuUsername(attributes["slug"].toString().toStdString());

    if (authenticated) {
      taiga::accounts.setKitsuEmail(attributes["email"].toString().toStdString());
      taiga::accounts.setKitsuRatingSystem(attributes["ratingSystem"].toString().toStdString());
      emit authenticationCompleted(true);
    }

    if (onSuccess) onSuccess();
  };

  manager_.get(api_.createRequest(u"/users"_s, query), this, callback);
}

}  // namespace sync::kitsu
