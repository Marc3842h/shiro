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

#include <curl/curl.h>

#include "../thirdparty/loguru.hh"
#include "curler.hh"

std::tuple<bool, std::string> shiro::utils::curl::get(const std::string &url) {
    CURL *curl = curl_easy_init();
    CURLcode status_code;

    if (curl == nullptr)
        return { false, "Unable to acquire curl handle." };

    std::string output;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "shiro (https://github.com/Marc3842h/shiro)");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    status_code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (status_code == CURLE_OK)
        return { true, output };

    return { false, curl_easy_strerror(status_code) };
}

size_t shiro::utils::curl::internal_callback(void *raw_data, size_t size, size_t memory, std::string *ptr) {
    size_t new_length = size * memory;
    size_t old_length = ptr->size();

    try {
        ptr->resize(old_length + new_length);
    } catch (const std::bad_alloc &ex) {
        LOG_F(ERROR, "Unable to allocate new memory for http response: %s.", ex.what());
        return 0;
    }

    std::copy((char*) raw_data, (char*) raw_data + new_length, ptr->begin() + old_length);
    return size * memory;
}