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

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include "../../config/cli_args.hh"
#include "../../native/process_info.hh"
#include "../../permissions/role_manager.hh"
#include "../../thirdparty/loguru.hh"
#include "../../utils/bot_utils.hh"
#include "restart_command.hh"

bool shiro::commands::restart(std::deque<std::string> &args, std::shared_ptr<shiro::users::user> user, std::string channel) {
    if (!roles::manager::has_permission(user, permissions::perms::cmd_restart)) {
        utils::bot::respond("Permission denied. (" + std::to_string((uint64_t) permissions::perms::cmd_restart) + ")", std::move(user), std::move(channel), true);
        return false;
    }

    auto [_, argv] = config::cli::get_args();

    // Close all file descriptors except stdin (0), stdout (1) and stderr (2)
    for (int64_t i = 3; i < sysconf(_SC_OPEN_MAX); i++) {
        close(i);
    }

    execve(native::process_info::get_executable_location().c_str(), argv, nullptr);

    // execve does not return if it runs successful.
    // We can't notify the user as we closed all file descriptors above, so we'll abort
    ABORT_F("Unable to fully restart Shiro: %s", std::strerror(errno));
}