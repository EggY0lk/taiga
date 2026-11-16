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

#include "kitsu_utils.hpp"

#include <QJsonObject>
#include <QJsonValue>
#include <QRestReply>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
#include <format>

#include "base/string.hpp"
#include "media/anime.hpp"
#include "media/anime_season.hpp"
#include "sync/kitsu/kitsu_parsers.hpp"
#include "sync/kitsu/kitsu_ratings.hpp"
#include "sync/search_params.hpp"

namespace sync::kitsu {

bool isTransientError(const QRestReply& reply) {
  if (reply.hasError()) return true;

  const auto status = reply.httpStatus();
  return status == 403 || status == 408 || status == 429 || status >= 500;
}

int retryDelay(const int attempt) {
  return kRetryBaseDelayMs * (1 << std::min(attempt, 4));
}

QString fromSearchParams(const SearchParams& params) {
  // Text search already returns results in relevance order.
  if (!params.text.isEmpty()) return {};

  if (params.sort) {
    const auto prefix = params.sortOrder == Qt::SortOrder::DescendingOrder ? u"-"_s : QString{};
    // clang-format off
    switch (*params.sort) {
      case SearchSort::Title: return prefix + u"canonicalTitle"_s;
      case SearchSort::Duration: return prefix + u"episodeLength"_s;
      case SearchSort::Score: return prefix + u"averageRating"_s;
      case SearchSort::Type: return prefix + u"subtype"_s;
      case SearchSort::StartDate: return prefix + u"startDate"_s;
    }
    // clang-format on
  }

  // Without an explicit sort, season browsing returns inconsistent ordering
  // and duplicate objects across pages.
  return u"-user_count"_s;
}

QString fromSeasonName(const anime::SeasonName name) {
  // clang-format off
  switch (name) {
    case anime::SeasonName::Unknown: return "";
    case anime::SeasonName::Winter: return "winter";
    case anime::SeasonName::Spring: return "spring";
    case anime::SeasonName::Summer: return "summer";
    case anime::SeasonName::Fall: return "fall";
  }
  // clang-format on
  return "";
}

QString fromStatus(const anime::Status value) {
  // clang-format off
  switch (value) {
    case anime::Status::Unknown: return "";
    case anime::Status::Airing: return "current";
    case anime::Status::FinishedAiring: return "finished";
    case anime::Status::NotYetAired: return "tba,unreleased,upcoming";
  }
  // clang-format on
  return "";
}

QString fromType(const anime::Type value) {
  // clang-format off
  switch (value) {
    case anime::Type::Unknown: return "";
    case anime::Type::Tv: return "TV";
    case anime::Type::Ova: return "OVA";
    case anime::Type::Movie: return "movie";
    case anime::Type::Special: return "special";
    case anime::Type::Ona: return "ONA";
    case anime::Type::Music: return "music";
  }
  // clang-format on
  return "";
}

////////////////////////////////////////////////////////////////////////////////

QJsonObject buildLibraryEntryObject(const anime::list::Entry& entry,
                                    const anime::list::Fields dirty, const QString& userId) {
  using anime::list::Field;

  QJsonObject attributes;

  // Note that we send `null` (rather than `0` or `""`) to remove certain values.
  if (dirty & Field::Episode) {
    attributes["progress"] = entry.watchedEpisodes;
  }
  if (dirty & Field::Score) {
    attributes["ratingTwenty"] =
        entry.score > 0 ? QJsonValue(fromListScore(entry.score)) : QJsonValue{QJsonValue::Null};
  }
  if (dirty & Field::Status) {
    attributes["status"] = fromListStatus(entry.status);
  }
  if (dirty & Field::Rewatching) {
    attributes["reconsuming"] = entry.rewatching;
  }
  if (dirty & Field::RewatchedTimes) {
    attributes["reconsumeCount"] = entry.rewatchedTimes;
  }
  if (dirty & Field::DateStarted) {
    attributes["startedAt"] =
        entry.dateStarted ? fromListDate(QString::fromStdString(entry.dateStarted.toString()))
                          : QJsonValue{QJsonValue::Null};
  }
  if (dirty & Field::DateCompleted) {
    attributes["finishedAt"] =
        entry.dateCompleted ? fromListDate(QString::fromStdString(entry.dateCompleted.toString()))
                            : QJsonValue{QJsonValue::Null};
  }
  if (dirty & Field::Notes) {
    attributes["notes"] = QString::fromStdString(entry.notes);
  }

  QJsonObject animeData{
      {"type", "anime"},
      {"id", QString::number(entry.animeId)},
  };

  QJsonObject userData{
      {"type", "users"},
      {"id", userId},
  };

  QJsonObject data{
      {"type", "libraryEntries"},
      {"attributes", attributes},
      {"relationships",
       QJsonObject{
           {"anime", QJsonObject{{"data", animeData}}},
           {"user", QJsonObject{{"data", userData}}},
       }},
  };

  if (entry.id != anime::list::kUnknownId) {
    data["id"] = QString::number(entry.id);
  }

  return QJsonObject{{"data", data}};
}

std::optional<int> pagingOffset(const QJsonObject& links, const QString& key) {
  // `QUrlQuery(QUrl)` keeps percent-encoded keys (e.g. `page%5Boffset%5D`) encoded, so the query
  // items are requested in fully decoded form instead.
  const QUrlQuery query{QUrl{links[key].toString()}};

  for (const auto& [name, value] : query.queryItems(QUrl::FullyDecoded)) {
    if (name == u"page[offset]"_s) {
      bool ok = false;
      const auto offset = value.toInt(&ok);
      return ok ? std::make_optional(offset) : std::nullopt;
    }
  }

  return std::nullopt;
}

////////////////////////////////////////////////////////////////////////////////

QString animeFields(const bool minimal) {
  auto fields =
      // attributes
      u"abbreviatedTitles,"
      "ageRating,"
      "averageRating,"
      "canonicalTitle,"
      "endDate,"
      "episodeCount,"
      "episodeLength,"
      "popularityRank,"
      "posterImage,"
      "slug,"
      "startDate,"
      "status,"
      "subtype,"
      "titles,"
      "youtubeVideoId,"
      // relationships
      "animeProductions,"
      "categories"_s;

  if (!minimal) fields += u",synopsis"_s;

  return fields;
}

QString libraryEntryFields() {
  return
      // attributes
      u"finishedAt,"
      "notes,"
      "private,"
      "progress,"
      "ratingTwenty,"
      "reconsumeCount,"
      "reconsuming,"
      "startedAt,"
      "status,"
      "updatedAt,"
      // relationships
      "anime"_s;
}

QString userFields() {
  return u"email,"
         "name,"
         "ratingSystem,"
         "slug"_s;
}

std::string animePageUrl(const int id) {
  return std::format("https://kitsu.app/anime/{}", id);
}

}  // namespace sync::kitsu
