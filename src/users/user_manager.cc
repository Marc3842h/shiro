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

#include <algorithm>

#include "../database/tables/user_table.hh"
#include "../thirdparty/loguru.hh"
#include "user_manager.hh"

shiro::utils::thread_safe::locked_vector<std::shared_ptr<shiro::users::user>> shiro::users::manager::online_users;
//std::vector<std::shared_ptr<shiro::users::user>> shiro::users::manager::online_users;

void shiro::users::manager::login_user(std::shared_ptr<shiro::users::user> user) {
    if (user == nullptr || user->token.empty())
        return;

    if (is_online(user))
        logout_user(user);

    auto [users, lock] = online_users.get();

    users.emplace_back(user);

    LOG_F(INFO, "User %s logged in successfully.", user->presence.username.c_str());
}

void shiro::users::manager::logout_user(std::shared_ptr<shiro::users::user> user) {
    if (user == nullptr || !is_online(user))
        return;

    auto [users, lock] = online_users.get();

    auto iterator = std::find(users.begin(), users.end(), user);

    if (iterator == users.end())
        return;

	users.erase(iterator);

    LOG_F(INFO, "User %s logged out successfully.", user->presence.username.c_str());
}

void shiro::users::manager::logout_user(int32_t user_id) {
    if (!is_online(user_id))
        return;

    std::shared_ptr<user> target_user = nullptr;

    for (const std::shared_ptr<user> &user : online_users.iterable()) {
        if (user->user_id == user_id) {
            target_user = user;
            break;
        }
    }

    if (target_user == nullptr)
        return;

    logout_user(target_user);
}

bool shiro::users::manager::is_online(std::shared_ptr<shiro::users::user> user) {
    if (user == nullptr)
        return false;

    auto [users, lock] = online_users.get();
    return std::find(users.begin(), users.end(), user) != users.end();
}

bool shiro::users::manager::is_online(int32_t user_id) {
    for (const std::shared_ptr<user> &user : online_users.iterable()) {
        if (user->user_id == user_id)
            return true;
    }

    return false;
}

bool shiro::users::manager::is_online(const std::string &token) {
    if (token.empty())
        return false;

    for (const std::shared_ptr<user> &user : online_users.iterable()) {
        if (user->token == token)
            return true;
    }

    return false;
}

std::shared_ptr<shiro::users::user> shiro::users::manager::get_user_by_username(const std::string &username) {
    if (username.empty())
        return nullptr;

    for (const std::shared_ptr<user> &user : online_users.iterable()) {
        if (user->presence.username == username)
            return user;
    }

    return nullptr;
}

std::shared_ptr<shiro::users::user> shiro::users::manager::get_user_by_id(int32_t id) {
    if (!is_online(id))
        return nullptr;

    for (const std::shared_ptr<user> &user : online_users.iterable()) {
        if (user->user_id == id)
            return user;
    }

    return nullptr;
}

std::shared_ptr<shiro::users::user> shiro::users::manager::get_user_by_token(const std::string &token) {
    if (token.empty())
        return nullptr;

    if (!is_online(token))
        return nullptr;

    for (const std::shared_ptr<user> &user : online_users.iterable()) {
        if (user->token == token)
            return user;
    }

    return nullptr;
}

std::string shiro::users::manager::get_username_by_id(int32_t id) {
    std::shared_ptr<user> user = get_user_by_id(id);

    if (user != nullptr)
        return user->presence.username;

    sqlpp::mysql::connection db(db_connection->get_config());
    const tables::users user_table{};

    auto result = db(select(all_of(user_table)).from(user_table).where(user_table.id == id));
    bool empty = is_query_empty(result);

    if (empty)
        return "";

    for (const auto &row : result) {
        return row.username;
    }

    return "";
}

int32_t shiro::users::manager::get_id_by_username(const std::string &username) {
    std::shared_ptr<user> user = get_user_by_username(username);

    if (user != nullptr)
        return user->user_id;

    sqlpp::mysql::connection db(db_connection->get_config());
    const tables::users user_table{};

    auto result = db(select(all_of(user_table)).from(user_table).where(user_table.username == username));
    bool empty = is_query_empty(result);

    if (empty)
        return -1;

    for (const auto &row : result) {
        return row.id;
    }

    return -1;
}

size_t shiro::users::manager::get_online_users() {
    auto users = online_users.non_locked_get();
    return users.size();
}
