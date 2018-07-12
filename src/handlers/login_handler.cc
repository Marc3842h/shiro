#include "../io/layouts/packets.hh"
#include "../io/osu_writer.hh"
#include "../thirdparty/digestpp.hh"
#include "../thirdparty/loguru.hh"
#include "../thirdparty/uuid.hh"
#include "../users/user.hh"
#include "../users/user_manager.hh"
#include "../utils/string_utils.hh"
#include "login_handler.hh"

void shiro::handler::login::handle(const crow::request &request, crow::response &response) {
    std::string body = request.body;

    if (body.empty()) {
        response.end();
        return;
    }

    std::vector<std::string> lines = utils::strings::split(body, '\n');

    if (lines.size() != 4) {
        response.code = 403;
        response.end();

        LOG_F(WARNING, "Received invalid login request from %s: Login body has wrong length.", request.get_header_value("X-Forwarded-For").c_str());
        return;
    }

    std::string username = lines.at(0);
    std::string password_md5 = lines.at(1);

    std::vector<std::string> additional_info = utils::strings::split(lines.at(2), '|');

    if (additional_info.size() != 5) {
        response.code = 403;
        response.end();

        LOG_F(WARNING, "Received invalid login request from %s: Additional info has wrong length.", request.get_header_value("X-Forwarded-For").c_str());
        return;
    }

    io::osu_writer writer;
    writer.protocol_negotiation(shiro::io::cho_protocol);

    std::shared_ptr<users::user> user = std::make_shared<users::user>(username);

    if (!user->init()) {
        writer.login_reply(-1);

        response.end(writer.serialize());

        LOG_F(WARNING, "%s (%s) tried to login as non-existent user.", username.c_str(), request.get_header_value("X-Forwarded-For").c_str());
        return;
    }

    std::string version = additional_info.at(0);
    std::string utc_offset = additional_info.at(1);
    std::string hwid = additional_info.at(3);

    if (!user->check_password(password_md5)) {
        writer.login_reply(-1);

        response.end(writer.serialize());

        LOG_F(WARNING, "%s (%s) tried to login with wrong password.", username.c_str(), request.get_header_value("X-Forwarded-For").c_str());
        return;
    }

    user->token = sole::uuid4().str();
    user->client_version = version;
    user->utc_offset = utc_offset;
    user->hwid = digestpp::sha256().absorb(hwid).hexdigest();

    users::manager::login_user(user);

    writer.login_reply(user->user_id);
    writer.login_permissions(user->presence.permissions);
    writer.announce("Welcome to shiro!");

    response.end(writer.serialize());
}
