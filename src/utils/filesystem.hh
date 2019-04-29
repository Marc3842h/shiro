/*
 * shiro - High performance, high quality osu!Bancho C++ re-implementation
 * Copyright (C) 2018-2019 Marc3842h, czapek
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

#ifndef SHIRO_FILESYSTEM_UTILS
#define SHIRO_FILESYSTEM_UTILS

#include <filesystem>

#if defined(_WIN32)
    // Workaround for Windows: https://docs.microsoft.com/en-us/cpp/standard-library/filesystem
    namespace fs = std::experimental::filesystem::v1;
#else
    namespace fs = std::filesystem;
#endif

namespace shiro::utils::filesystem {

    // Fix for MSVC's stl prefering wchar_t over char
    inline constexpr char preferred_separator = (char) fs::path::preferred_separator;

    inline std::string combine(const std::string &a, const std::string &b) {
        return a + preferred_separator + b;
    }

}

#endif //SHIRO_FILESYSTEM_UTILS
