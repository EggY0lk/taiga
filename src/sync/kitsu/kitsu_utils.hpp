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

#pragma once

#include <QString>
#include <optional>
#include <string>

#include "media/anime_list.hpp"

class QJsonObject;
class QRestReply;

namespace anime {
enum class SeasonName;
enum class Status;
enum class Type;
}  // namespace anime

namespace sync {
struct SearchParams;
}

namespace sync::kitsu {

// Cloudflare sits in front of kitsu.app and occasionally answers with a 403 challenge, a 429
// rate limit, or a timeout instead of the expected response. These are transient, so requests
// are retried with a growing delay rather than ending the whole synchronization.
inline constexpr int kMaxRetries = 5;
inline constexpr int kRetryBaseDelayMs = 1500;

// A short pause between library pages keeps Kitsu's rate limiter happy.
inline constexpr int kPageDelayMs = 150;

bool isTransientError(const QRestReply& reply);
int retryDelay(const int attempt);

QString fromSearchParams(const SearchParams& params);
QString fromSeasonName(const anime::SeasonName name);
QString fromStatus(const anime::Status value);
QString fromType(const anime::Type value);

QJsonObject buildLibraryEntryObject(const anime::list::Entry& entry,
                                    const anime::list::Fields dirty, const QString& userId);
std::optional<int> pagingOffset(const QJsonObject& links, const QString& key);

// Sparse fieldsets
// https://jsonapi.org/format/#fetching-sparse-fieldsets
QString animeFields(const bool minimal = false);
QString libraryEntryFields();
QString userFields();

std::string animePageUrl(const int id);

}  // namespace sync::kitsu
