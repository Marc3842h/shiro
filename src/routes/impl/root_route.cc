#include <string>

#include "../../handlers/login_handler.hh"
#include "../../io/osu_reader.hh"
#include "../../io/osu_writer.hh"
#include "../../thirdparty/loguru.hh"
#include "../../users/user_manager.hh"
#include "../../views/index_view.hh"
#include "../../shiro.hh"
#include "../packets/packet_router.hh"
#include "root_route.hh"

void shiro::routes::root::handle(const crow::request &request, crow::response &response) {
    response.set_header("Content-Type", "text/html; charset=UTF-8");
    response.set_header("cho-server", "shiro (https://github.com/Marc3842h/shiro)");

    if (request.method == crow::HTTPMethod::Get) {
        std::string view = shiro::views::index::get_view();

        if (view.empty()) {
            response.code = 500;
            response.end();
            return;
        }

        response.end(view);
        return;
    }

    // Generic response metadata
    response.set_header("Keep-Alive", "timeout=5, max=100");
    response.set_header("Content-Type", "application/octet-stream; charset=UTF-8");

    const std::string &user_agent = request.get_header_value("user-agent");

    if (user_agent.empty() || user_agent != "osu!") {
        response.code = 403;
        response.end();

        LOG_F(WARNING, "Received POST from %s without osu! user agent.", request.get_header_value("X-Forwarded-For").c_str());
        return;
    }

    // osu! metadata
    response.set_header("cho-protocol", std::to_string(shiro::io::cho_protocol));
    response.set_header("cho-token", "");

    if (request.get_header_value("osu-token").length() <= 0) {
        handler::login::handle(request, response);
        return;
    }

    std::shared_ptr<users::user> user = users::manager::get_user_by_token(request.get_header_value("osu-token"));

    if (user == nullptr) {
        response.code = 403;
        response.end();

        LOG_F(WARNING, "%s sent a request with a invalid osu token.", request.get_header_value("X-Forwarded-For").c_str());
        return;
    }

    io::osu_writer writer;
    io::osu_reader reader(request.body);

    for (io::osu_packet &packet : reader.parse()) {
        routes::packets::route(packet.id, packet, writer, user);
    }

    std::string result = writer.serialize();

    if (!user->queue.is_empty())
        result.append(user->queue.serialize());

    response.end(result);
}
