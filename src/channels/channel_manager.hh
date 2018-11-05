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

#ifndef SHIRO_CHANNEL_MANAGER_HH
#define SHIRO_CHANNEL_MANAGER_HH

#include <unordered_map>

#include "../io/layouts/channel/channel.hh"
#include "../io/osu_writer.hh"
#include "../users/user.hh"

namespace shiro::channels::manager {

    extern std::unordered_map<io::layouts::channel, std::vector<std::shared_ptr<users::user>>> channels;

    void init();

    void write_channels(io::osu_writer &buf, std::shared_ptr<shiro::users::user> user, bool first = true);

    // User methods

    void join_channel(uint32_t channel_id, std::shared_ptr<users::user> user);

    void leave_channel(uint32_t channel_id, std::shared_ptr<users::user> user);

    bool in_channel(uint32_t channel_id, const std::shared_ptr<users::user> &user);

    std::vector<std::shared_ptr<users::user>> get_users_in_channel(const std::string &channel_name);

    uint32_t get_channel_id(const std::string &channel_name);
}

#endif //SHIRO_CHANNEL_MANAGER_HH
