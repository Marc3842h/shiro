/*
 * shiro - High performance, high quality osu!Bancho C++ re-implementation
 * Copyright (C) 2018 Marc3842h, czapek
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SHIRO_GEOLOC_HH
#define SHIRO_GEOLOC_HH

#include <string>

#include "../thirdparty/cache/cache.hh"
#include "location_info.hh"

namespace shiro::geoloc {

    // Limit cache to 256 locations, oldest ones will be overriden once we go over this
    extern caches::fixed_sized_cache<std::string, location_info> location_cache;

    location_info get_location(const std::string &ip_address);

    size_t callback(void *raw_data, size_t size, size_t memory, std::string *ptr);
}

#endif //SHIRO_GEOLOC_HH
