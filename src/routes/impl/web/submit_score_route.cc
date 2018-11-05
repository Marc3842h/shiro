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

#define OPPAI_IMPLEMENTATION

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "../../../beatmaps/beatmap.hh"
#include "../../../beatmaps/beatmap_helper.hh"
#include "../../../config/score_submission_file.hh"
#include "../../../database/tables/score_table.hh"
#include "../../../ranking/ranking_helper.hh"
#include "../../../replays/replay_manager.hh"
#include "../../../scores/score.hh"
#include "../../../scores/score_helper.hh"
#include "../../../thirdparty/loguru.hh"
#include "../../../thirdparty/oppai.hh"
#include "../../../users/user.hh"
#include "../../../users/user_manager.hh"
#include "../../../users/user_punishments.hh"
#include "../../../utils/bot_utils.hh"
#include "../../../utils/crypto.hh"
#include "../../../utils/multipart_parser.hh"
#include "../../../utils/string_utils.hh"
#include "submit_score_route.hh"

void shiro::routes::web::submit_score::handle(const crow::request &request, crow::response &response) {
    response.set_header("Content-Type", "text/plain; charset=UTF-8");
    response.set_header("cho-server", "shiro (https://github.com/Marc3842h/shiro)");

    const std::string &user_agent = request.get_header_value("user-agent");

    if (user_agent.empty() || user_agent != "osu!") {
        response.code = 403;
        response.end();

        LOG_F(WARNING, "Received score submission from %s without osu! user agent.", request.get_ip_address().c_str());
        return;
    }

    const std::string &content_type = request.get_header_value("content-type");

    if (content_type.empty()) {
        response.code = 400;
        response.end("error: invalid");

        LOG_S(ERROR) << "Received score submission without content type.";
        return;
    }

    std::unique_ptr<utils::multipart_parser> parser = std::make_unique<utils::multipart_parser>(request.body, content_type);

    if (parser == nullptr)
        return;

    utils::multipart_form_fields fields = parser->parse_fields();
    std::string key = "h89f2-890h2h89b34g-h80g134n90133";

    if (fields.find("pass") == fields.end()) {
        response.code = 403;
        response.end("error: pass");

        LOG_S(WARNING) << "Received score submission without password.";
        return;
    }

    if (fields.find("iv") == fields.end()) {
        response.code = 400;
        response.end("error: invalid");

        LOG_S(WARNING) << "Received score without initialization vector.";
        return;
    }

    if (fields.find("score") == fields.end()) {
        response.code = 400;
        response.end("error: invalid");

        LOG_S(WARNING) << "Received score without score data.";
        return;
    }

    if (fields.find("osuver") != fields.end())
        key = "osu!-scoreburgr---------" + fields.at("osuver").body;

    std::vector<unsigned char> decrypted = utils::crypto::rijndael256::decode(
        utils::crypto::base64::decode(fields.at("iv").body.c_str()),
        key,
        utils::crypto::base64::decode(fields.at("score").body.c_str()));

    if (decrypted.empty()) {
        response.code = 400;
        response.end("error: invalid");

        LOG_S(WARNING) << "Received score without score metadata.";
        return;
    }

    std::string result(reinterpret_cast<char *>(&decrypted[0]), decrypted.size());

    std::vector<std::string> score_metadata;
    boost::split(score_metadata, result, boost::is_any_of(":"));

    if (score_metadata.size() < 18) {
        response.code = 400;
        response.end("error: invalid");

        LOG_S(WARNING) << "Received invalid score submission, score metadata doesn't have 16 or more parts.";
        return;
    }

    boost::trim_right(score_metadata.at(1));

    std::shared_ptr<users::user> user = users::manager::get_user_by_username(score_metadata.at(1));

    // This only occurs when the server restarted and osu submitted before being re-logged in
    if (user == nullptr) {
        response.code = 403;

        LOG_S(WARNING) << "Received score submission from offline user.";
        return;
    }

    if (!user->check_password(fields.at("pass").body)) {
        response.code = 403;
        response.end("error: pass");

        LOG_F(WARNING, "Received score submission from %s with incorrect password.", user->presence.username.c_str());
        return;
    }

    if (users::punishments::is_banned(user->user_id)) {
        response.code = 403;
        response.end("error: no");

        LOG_F(WARNING, "Received score submission from %s while user is banned.", user->presence.username.c_str());
        return;
    }

    sqlpp::mysql::connection db(db_connection->get_config());
    const tables::scores score_table{};

    int32_t game_version = 20131216;

    score_metadata.at(17).erase(std::remove_if(score_metadata.at(17).begin(), score_metadata.at(17).end(), [](char c) {
        return !std::isdigit(c);
    }),
        score_metadata.at(17).end());

    scores::score score;
    score.user_id = user->user_id;

    score.beatmap_md5 = score_metadata.at(0);
    score.hash = score_metadata.at(2);

    try {
        score._300_count = boost::lexical_cast<int32_t>(score_metadata.at(3));
        score._100_count = boost::lexical_cast<int32_t>(score_metadata.at(4));
        score._50_count = boost::lexical_cast<int32_t>(score_metadata.at(5));
        score.gekis_count = boost::lexical_cast<int32_t>(score_metadata.at(6));
        score.katus_count = boost::lexical_cast<int32_t>(score_metadata.at(7));
        score.miss_count = boost::lexical_cast<int32_t>(score_metadata.at(8));
        score.total_score = boost::lexical_cast<int64_t>(score_metadata.at(9));
        score.max_combo = boost::lexical_cast<int32_t>(score_metadata.at(10));
        score.mods = boost::lexical_cast<int32_t>(score_metadata.at(13));
        score.play_mode = static_cast<uint8_t>(boost::lexical_cast<int32_t>(score_metadata.at(15)));
    } catch (const boost::bad_lexical_cast &ex) {
        response.code = 500;
        response.end();

        LOG_F(WARNING, "Received score submission from %s with invalid types.", user->presence.username.c_str());
        return;
    }

    try {
        game_version = boost::lexical_cast<int32_t>(score_metadata.at(17));
    } catch (const boost::bad_lexical_cast &ex) {
        LOG_S(WARNING) << "Unable to convert " << score_metadata.at(17) << " to game version: " << ex.what();

        // Give the client a chance to resubmit so the player doesn't get restricted for a fail on our side.
        if (config::score_submission::restrict_mismatching_client_version) {
            response.code = 500;
            response.end();
            return;
        }
    }

    if (score.play_mode > 3) {
        response.end("error: invalid");

        LOG_F(WARNING, "%s submitted a score with a invalid play mode.", user->presence.username.c_str());
        return;
    }

    std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());

    score.rank = score_metadata.at(12);
    score.fc = utils::strings::to_bool(score_metadata.at(11));
    score.passed = utils::strings::to_bool(score_metadata.at(14));
    score.time = seconds.count();

    score.accuracy = scores::helper::calculate_accuracy(
        (utils::play_mode) score.play_mode,
        score._300_count,
        score._100_count,
        score._50_count,
        score.gekis_count,
        score.katus_count,
        score.miss_count);

    auto db_result = db(select(all_of(score_table)).from(score_table).where(score_table.hash == score.hash));
    bool empty = is_query_empty(db_result);

    // Score has already been submitted
    if (!empty) {
        response.end("error: dup");

        LOG_F(WARNING, "%s resubmitted a previously submitted score.", user->presence.username.c_str());
        return;
    }

    user->stats.play_count++;

    beatmaps::beatmap beatmap;
    beatmap.beatmap_md5 = score.beatmap_md5;

    beatmap.fetch();

    if (score.passed)
        beatmap.pass_count++;

    beatmap.play_count++;
    beatmap.update_play_metadata();

    if (fields.find("replay-bin") == fields.end()) {
        response.code = 400;
        response.end("error: invalid");

        if (config::score_submission::restrict_no_replay)
            users::punishments::restrict(user->user_id, "No replay sent on score submission");

        LOG_S(WARNING) << "Received score without replay data.";
        return;
    }

    std::vector<scores::score> previous_scores = scores::helper::fetch_user_scores(beatmap.beatmap_md5, user);
    bool overwrite = true;

    // User has previous scores on this map, enable overwriting mode
    if (!previous_scores.empty()) {
        for (const scores::score &s : previous_scores) {
            double factor_score;
            double factor_iterator;

            if (config::score_submission::overwrite_factor == "score") {
                factor_score = score.total_score;
                factor_iterator = s.total_score;
            } else if (config::score_submission::overwrite_factor == "accuracy") {
                factor_score = score.accuracy;
                factor_iterator = s.accuracy;
            } else { // pp
                factor_score = score.pp;
                factor_iterator = s.pp;
            }

            if (factor_iterator > factor_score)
                overwrite = false;
        }
    }

    if (!score.passed)
        overwrite = false;

    // oppai-ng for std and taiko (non-converted)
    // TODO: Replace with libakame (https://github.com/Marc3842h/libakame)
    if ((score.play_mode == (uint8_t) utils::play_mode::standard || score.play_mode == (uint8_t) utils::play_mode::taiko) &&
        beatmaps::helper::awards_pp(beatmaps::helper::fix_beatmap_status(beatmap.ranked_status))) {
        struct parser parser_state;
        struct beatmap map;

        struct diff_calc difficulty;
        struct pp_calc pp;

        struct pp_params params;

        p_init(&parser_state);
        p_map(&parser_state, &map, beatmaps::helper::download(beatmap.beatmap_id));

        d_init(&difficulty);
        d_calc(&difficulty, &map, score.mods);

        params.aim = difficulty.aim;
        params.speed = difficulty.speed;
        params.max_combo = beatmap.max_combo;
        params.nsliders = map.nsliders;
        params.ncircles = map.ncircles;
        params.nobjects = map.nobjects;

        params.mode = score.play_mode;
        params.mods = score.mods;
        params.combo = score.max_combo;
        params.n300 = score._300_count;
        params.n100 = score._100_count;
        params.n50 = score._50_count;
        params.nmiss = score.miss_count;
        params.score_version = PP_DEFAULT_SCORING;

        ppv2p(&pp, &params);

        score.pp = pp.total;

        d_free(&difficulty);
        p_free(&parser_state);
    } else {
        score.pp = 0;
    }

    // Auto restriction for weird things enabled in score_submission.toml
    auto [flagged, reason] = scores::helper::is_flagged(score, beatmap);

    if (flagged)
        users::punishments::restrict(user->user_id, reason);

    // Auto restriction for bad replay submitters that submit without editing username
    if (config::score_submission::restrict_mismatching_username && score_metadata.at(1) != user->presence.username)
        users::punishments::restrict(user->user_id, "Mismatching username on score submission (" + score_metadata.at(1) + " != " + user->presence.username + ")");

    // ...or the client build
    if (config::score_submission::restrict_mismatching_client_version && game_version != user->client_build)
        users::punishments::restrict(user->user_id, "Mismatching client version on score submission (" + std::to_string(game_version) + " != " + std::to_string(user->client_build) + ")");

    // Auto restriction for notepad hack
    if (config::score_submission::restrict_notepad_hack && fields.find("bmk") != fields.end() && fields.find("bml") != fields.end()) {
        std::string bmk = fields.at("bmk").body;
        std::string bml = fields.at("bml").body;

        if (bmk != bml)
            users::punishments::restrict(user->user_id, "Mismatching bmk and bml (notepad hack, " + bmk + " != " + bml + ")");
    }

    scores::score old_top_score = scores::helper::fetch_top_score_user(beatmap.beatmap_md5, user);
    int32_t old_scoreboard_pos = scores::helper::get_scoreboard_position(old_top_score, scores::helper::fetch_all_scores(beatmap.beatmap_md5, 5));

    if (old_scoreboard_pos == -1)
        old_scoreboard_pos = 0;

    db(insert_into(score_table)
            .set(
                score_table.user_id = score.user_id,
                score_table.hash = score.hash,
                score_table.beatmap_md5 = score.beatmap_md5,
                score_table.rank = score.rank,
                score_table.score = score.total_score,
                score_table.max_combo = score.max_combo,
                score_table.pp = score.pp,
                score_table.accuracy = score.accuracy,
                score_table.mods = score.mods,
                score_table.fc = score.fc,
                score_table.passed = score.passed,
                score_table._300_count = score._300_count,
                score_table._100_count = score._100_count,
                score_table._50_count = score._50_count,
                score_table.katus_count = score.katus_count,
                score_table.gekis_count = score.gekis_count,
                score_table.miss_count = score.miss_count,
                score_table.play_mode = score.play_mode,
                score_table.time = score.time));

    db_result = db(select(all_of(score_table)).from(score_table).where(score_table.hash == score.hash));
    empty = is_query_empty(db_result);

    if (empty) {
        response.end("error: invalid");
        return;
    }

    // Just to get the id
    for (const auto &row : db_result) {
        score.id = row.id;
    }

    if (overwrite)
        user->stats.total_score += score.total_score;

    replays::save_replay(score, beatmap, game_version, fields.at("replay-bin").body);

    if (!scores::helper::is_ranked(score, beatmap)) {
        response.end("ok" /*"error: disabled"*/);
        return;
    }

    if (!score.passed) {
        response.end("ok");
        return;
    }

    scores::score top_score = scores::helper::fetch_top_score_user(beatmap.beatmap_md5, user);
    int32_t scoreboard_position = scores::helper::get_scoreboard_position(top_score, scores::helper::fetch_all_scores(beatmap.beatmap_md5, 5));

    if (top_score.hash == score.hash && !user->hidden) {
        if (scoreboard_position == 1) {
            char buffer[1024];
            std::snprintf(
                buffer, sizeof(buffer), "[https://shiro.host/u/%i %s] achieved rank #1 on [https://osu.ppy.sh/b/%i %s] (%s)", user->user_id, user->presence.username.c_str(), beatmap.beatmap_id, beatmap.song_name.c_str(), utils::play_mode_to_string((utils::play_mode) score.play_mode).c_str());

            utils::bot::respond(buffer, user, "#announce", false);
        }
    }

    if (overwrite)
        user->stats.ranked_score += score.total_score;

    user->stats.recalculate_pp();

    float old_acc = user->stats.accuracy;
    user->stats.recalculate_accuracy();

    user->save_stats();

    int32_t old_rank = user->stats.rank;

    if (overwrite && !user->hidden)
        ranking::helper::recalculate_ranks(user->stats.play_mode);

    std::string user_above = ranking::helper::get_leaderboard_user(user->stats.play_mode, user->stats.rank - 1);
    int16_t user_above_pp = ranking::helper::get_pp_for_user(user->stats.play_mode, user_above);

    uint32_t timestamp = static_cast<uint32_t>(beatmap.last_update);
    std::time_t time = timestamp;

    int32_t to_next_rank = user_above_pp - user->stats.pp;

    if (to_next_rank < 0)
        to_next_rank = 0;

    struct std::tm *tm = std::gmtime(&time);
    std::stringstream out;

    out << std::setprecision(12);
    out << "beatmapId:" << beatmap.beatmap_id << "|";
    out << "beatmapSetId:" << beatmap.beatmapset_id << "|";
    out << "beatmapPlaycount:" << beatmap.play_count << "|";
    out << "beatmapPasscount:" << beatmap.pass_count << "|";
    out << "approvedDate:" << std::put_time(tm, "%F %X") << std::endl;
    out << "chartId:overall"
        << "|";
    out << "chartName:Overall Ranking"
        << "|";
    out << "chartEndDate:"
        << "|";
    out << "beatmapRankingBefore:" << old_scoreboard_pos << "|";
    out << "beatmapRankingAfter:" << scoreboard_position << "|";
    out << "rankedScoreBefore:" << user->stats.ranked_score - score.total_score << "|";
    out << "rankedScoreAfter:" << user->stats.ranked_score << "|";
    out << "totalScoreBefore:" << user->stats.total_score - score.total_score << "|";
    out << "totalScoreAfter:" << user->stats.total_score << "|";
    out << "playCountBefore:" << user->stats.play_count << "|";
    out << "accuracyBefore:" << (old_acc / 100) << "|";
    out << "accuracyAfter:" << (user->stats.accuracy / 100) << "|";
    out << "rankBefore:" << old_rank << "|";
    out << "rankAfter:" << user->stats.rank << "|";
    out << "toNextRank:" << to_next_rank << "|";
    out << "toNextRankUser:" << user_above << "|";
    out << "achievements:"
        << "|";
    out << "achievements-new:"
        << "|";
    out << "onlineScoreId:" << score.id;
    out << std::endl;

    response.end(out.str());
}
