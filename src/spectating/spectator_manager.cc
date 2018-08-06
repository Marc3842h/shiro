#include "spectator_manager.hh"

std::vector<std::pair<std::shared_ptr<shiro::users::user>, std::shared_ptr<shiro::users::user>>> shiro::spectating::manager::currently_spectating;

void shiro::spectating::manager::start_spectating(std::shared_ptr<shiro::users::user> user, std::shared_ptr<shiro::users::user> target) {
    if (is_spectating(user))
        stop_spectating(user);

    currently_spectating.emplace_back(std::make_pair(user, target));
}

void shiro::spectating::manager::stop_spectating(std::shared_ptr<shiro::users::user> user) {
    if (!is_spectating(user))
        return;

    for (size_t i = 0; i < currently_spectating.size(); i++) {
        const auto pair = currently_spectating.at(i);

        if (pair.first == user) {
            currently_spectating.erase(currently_spectating.begin() + i);
            break;
        }
    }
}

bool shiro::spectating::manager::is_spectating(std::shared_ptr<shiro::users::user> user) {
    for (const auto &pair : currently_spectating) {
        if (pair.first == user)
            return true;
    }

    return false;
}

bool shiro::spectating::manager::is_being_spectated(std::shared_ptr<shiro::users::user> user) {
    for (const auto &pair : currently_spectating) {
        if (pair.second == user)
            return true;
    }

    return false;
}

std::vector<std::shared_ptr<shiro::users::user>> shiro::spectating::manager::get_spectators(std::shared_ptr<shiro::users::user> user) {
    std::vector<std::shared_ptr<shiro::users::user>> users;

    for (const auto &pair : currently_spectating) {
        if (pair.second == user)
            users.emplace_back(pair.first);
    }

    return users;
}

std::shared_ptr<shiro::users::user> shiro::spectating::manager::get_host(std::shared_ptr<shiro::users::user> user) {
    for (const auto &pair : currently_spectating) {
        if (pair.first == user)
            return pair.second;
    }

    return nullptr;
}
