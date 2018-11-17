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

#include "../../channels/channel_manager.hh"
#include "../../users/user_manager.hh"
#include "leave_channel_handler.hh"

void shiro::handler::chat::leave::handle(shiro::io::osu_packet &in, shiro::io::osu_writer &out, std::shared_ptr<shiro::users::user> user) {
    std::string channel = in.data.read_string();
    uint32_t target_channel = channels::manager::get_channel_id(channel);

    if (target_channel == 0)
        return;

    io::osu_writer writer;

    if (!channels::manager::leave_channel(target_channel, user)) {
        writer.channel_join(channel);
        return;
    }

    channels::manager::write_channels(writer, user, false);

    for (const std::shared_ptr<users::user> &online_user : users::manager::online_users.iterable()) {
        if (online_user->user_id == 1)
            continue;

        online_user->queue.enqueue(writer);
    }

    out.channel_revoked(channel);
}
