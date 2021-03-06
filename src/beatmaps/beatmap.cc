/*
 * shiro - High performance, high quality osu!Bancho C++ re-implementation
 * Copyright (C) 2018-2020 Marc3842h, czapek
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

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <iomanip>

#include "../config/bancho_file.hh"
#include "../config/ipc_file.hh"
#include "../database/tables/beatmap_table.hh"
#include "../logger/sentry_logger.hh"
#include "../thirdparty/json.hh"
#include "../thirdparty/loguru.hh"
#include "../utils/curler.hh"
#include "../utils/string_utils.hh"
#include "../shiro.hh"
#include "beatmap.hh"
#include "beatmap_helper.hh"

void shiro::beatmaps::beatmap::fetch(bool force_peppster) {
    if (this->beatmapset_id == -1) {
        this->ranked_status = (int32_t) status::unsubmitted;
        return;
    }

    if (!force_peppster) {
        if (fetch_db())
            return;
    }

    if (!fetch_api())
        this->ranked_status = (int32_t) status::unknown;
}

bool shiro::beatmaps::beatmap::fetch_db() {
    sqlpp::mysql::connection db(db_connection->get_config());
    const tables::beatmaps beatmaps_table {};

    auto result = db(select(all_of(beatmaps_table)).from(beatmaps_table).where(beatmaps_table.beatmap_md5 == this->beatmap_md5).limit(1u));

    if (result.empty())
        return false;

    const auto &row = result.front();

    this->id = row.id;
    this->beatmap_id = row.beatmap_id;
    this->beatmapset_id = row.beatmapset_id;
    this->play_mode = row.game_mode;
    this->beatmap_md5 = row.beatmap_md5;
    this->song_name = row.song_name;
    this->ar = row.ar;
    this->od = row.od;
    this->size = row.size;
    this->drain = row.drain;
    this->aim = row.aim;
    this->speed = row.speed;
    this->diff_std = row.diff_std;
    this->diff_taiko = row.diff_taiko;
    this->diff_ctb = row.diff_ctb;
    this->diff_mania = row.diff_mania;
    this->max_combo = row.max_combo;
    this->hit_length = row.hit_length;
    this->bpm = row.bpm;
    this->ranked_status = row.ranked_status;
    this->ranked_status_freezed = row.ranked_status_freezed;
    this->last_update = row.last_update;
    this->play_count = row.play_count;
    this->pass_count = row.pass_count;

    return true;
}

bool shiro::beatmaps::beatmap::fetch_api() {
    if (this->beatmapset_id == 0) {
        std::string url = "https://old.ppy.sh/api/get_beatmaps?k=" + config::bancho::api_key + "&b=" + std::to_string(this->beatmap_id);
        auto [success, output] = utils::curl::get(url);

        if (!success) {
            LOG_F(ERROR, "Unable to connect to osu! api: %s.", output.c_str());

            this->ranked_status = (int32_t) status::unknown;
            return false;
        }

        json json_result;

        try {
            json_result = json::parse(output);
        } catch (const json::parse_error &ex) {
            LOG_F(ERROR, "Unable to parse json response from osu! api: %s.", ex.what());
            logging::sentry::exception(ex);

            this->ranked_status = (int32_t) status::unknown;
            return false;
        }

        for (auto &part : json_result) {
            try {
                this->beatmapset_id = boost::lexical_cast<int32_t>(std::string(part["beatmapset_id"]));
            } catch (const boost::bad_lexical_cast &ex) {
                LOG_F(ERROR, "Unable to cast response of osu! API to valid data types: %s.", ex.what());
                logging::sentry::exception(ex);

                this->ranked_status = (int32_t) status::unknown;
                return false;
            }
        }
    }

    std::string url = "https://old.ppy.sh/api/get_beatmaps?k=" + config::bancho::api_key + "&s=" + std::to_string(this->beatmapset_id);
    auto [success, output] = utils::curl::get(url);

    if (!success) {
        LOG_F(ERROR, "Unable to connect to osu! api: %s.", output.c_str());

        this->ranked_status = (int32_t) status::unknown;
        return false;
    }

    json json_result;
    std::string original_md5sum = this->beatmap_md5;
    bool map_found = false;

    try {
        json_result = json::parse(output);
    } catch (const json::parse_error &ex) {
        LOG_F(ERROR, "Unable to parse json response from osu! api: %s.", ex.what());
        logging::sentry::exception(ex);

        this->ranked_status = (int32_t) status::unknown;
        return false;
    }

    for (auto &part : json_result) {
        try {
            std::string artist = part["artist"];
            std::string title = part["title"];
            std::string version = part["version"];

            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "%s - %s [%s]", artist.c_str(), title.c_str(), version.c_str());
            this->song_name = buffer;

            this->beatmap_md5 = part["file_md5"];
            this->beatmapset_id = boost::lexical_cast<int32_t>(std::string(part["beatmapset_id"]));
            this->beatmap_id = boost::lexical_cast<int32_t>(std::string(part["beatmap_id"]));
            this->ranked_status = boost::lexical_cast<int32_t>(std::string(part["approved"]));
            this->hit_length = boost::lexical_cast<int32_t>(std::string(part["hit_length"]));
            this->play_mode = (uint8_t) boost::lexical_cast<int32_t>(std::string(part["mode"]));
            this->ar = boost::lexical_cast<float>(std::string(part["diff_approach"]));
            this->od = boost::lexical_cast<float>(std::string(part["diff_overall"]));
            this->size = boost::lexical_cast<float>(std::string(part["diff_size"]));
            this->drain = boost::lexical_cast<float>(std::string(part["diff_drain"]));
            this->bpm = static_cast<int32_t>(boost::lexical_cast<float>(std::string(part["bpm"])));

            switch ((utils::play_mode) this->play_mode) {
                case utils::play_mode::standard:
                    this->diff_std = boost::lexical_cast<float>(std::string(part["difficultyrating"]));
                    this->aim = boost::lexical_cast<float>(std::string(part["diff_aim"]));
                    this->speed = boost::lexical_cast<float>(std::string(part["diff_speed"]));

                    // For some older beatmaps the max_combo is null. See ppy/osu-api#130
                    if (!part["max_combo"].is_null())
                        this->max_combo = boost::lexical_cast<int32_t>(std::string(part["max_combo"]));

                    break;
                case utils::play_mode::taiko:
                    this->diff_taiko = boost::lexical_cast<float>(std::string(part["difficultyrating"]));
                    break;
                case utils::play_mode::fruits:
                    if (!part["difficultyrating"].is_null())
                        this->diff_ctb = boost::lexical_cast<float>(std::string(part["difficultyrating"]));

                    this->aim = boost::lexical_cast<float>(std::string(part["diff_aim"]));

                    // For some older beatmaps the max_combo is null. See ppy/osu-api#130
                    if (!part["max_combo"].is_null())
                        this->max_combo = boost::lexical_cast<int32_t>(std::string(part["max_combo"]));

                    break;
                case utils::play_mode::mania:
                    this->diff_mania = boost::lexical_cast<float>(std::string(part["difficultyrating"]));
                    break;
            }

            // Beatmap is unranked
            if (part["approved_date"].is_null())
                this->ranked_status = (int32_t) status::latest_pending;
        } catch (const boost::bad_lexical_cast &ex) {
            LOG_F(ERROR, "Unable to cast response of Bancho API to valid data types: %s.", ex.what());
            logging::sentry::exception(ex);

            this->ranked_status = (int32_t) status::unknown;
            return false;
        }

        if (this->beatmap_md5 == original_md5sum)
            map_found = true;

        std::string last_update = part["last_update"];

        std::tm time {};
        std::stringstream stream(last_update);
        stream >> std::get_time(&time, "%Y-%m-%d %H:%M:%S");
        std::chrono::time_point time_point = std::chrono::system_clock::from_time_t(std::mktime(&time));
        std::chrono::seconds seconds = std::chrono::time_point_cast<std::chrono::seconds>(time_point).time_since_epoch();
        this->last_update = seconds.count();

        save();
    }

    this->beatmap_md5 = original_md5sum;

    if (!map_found) {
        // Map was not found when queuing for the beatmap set, the map is not submitted.
        this->ranked_status = (int32_t) status::unsubmitted;
        return true;
    }

    fetch_db();
    return true;
}

void shiro::beatmaps::beatmap::save() {
    sqlpp::mysql::connection db(db_connection->get_config());
    const tables::beatmaps beatmaps_table {};

    db(insert_into(beatmaps_table).set(
            beatmaps_table.beatmap_id = this->beatmap_id,
            beatmaps_table.beatmapset_id = this->beatmapset_id,
            beatmaps_table.game_mode = this->play_mode,
            beatmaps_table.beatmap_md5 = this->beatmap_md5,
            beatmaps_table.song_name = this->song_name,
            beatmaps_table.ar = this->ar,
            beatmaps_table.od = this->od,
            beatmaps_table.size = this->size,
            beatmaps_table.drain = this->drain,
            beatmaps_table.aim = this->aim,
            beatmaps_table.speed = this->speed,
            beatmaps_table.diff_std = this->diff_std,
            beatmaps_table.diff_taiko = this->diff_taiko,
            beatmaps_table.diff_ctb = this->diff_ctb,
            beatmaps_table.diff_mania = this->diff_mania,
            beatmaps_table.max_combo = this->max_combo,
            beatmaps_table.hit_length = this->hit_length,
            beatmaps_table.bpm = this->bpm,
            beatmaps_table.ranked_status = this->ranked_status,
            beatmaps_table.ranked_status_freezed = this->ranked_status_freezed,
            beatmaps_table.last_update = this->last_update,
            beatmaps_table.play_count = this->play_count,
            beatmaps_table.pass_count = this->pass_count
    ));
}

void shiro::beatmaps::beatmap::update_play_metadata() {
    sqlpp::mysql::connection db(db_connection->get_config());
    const tables::beatmaps beatmaps_table {};

    db(update(beatmaps_table).set(
            beatmaps_table.play_count = this->play_count,
            beatmaps_table.pass_count = this->pass_count
    ).where(beatmaps_table.beatmap_md5 == this->beatmap_md5));
}

std::string shiro::beatmaps::beatmap::get_url() {
    std::string url = config::ipc::beatmap_url + std::to_string(this->beatmap_id);
    return url;
}

std::string shiro::beatmaps::beatmap::build_header() {
    std::stringstream result;

    result << helper::fix_beatmap_status(this->ranked_status) << "|false|" << this->beatmap_id << "|" << this->beatmapset_id << "|" << this->pass_count << std::endl;
    result << "0" << std::endl;
    result << this->song_name << std::endl;
    result << "10.0" << std::endl;

    return result.str();
}

std::string shiro::beatmaps::beatmap::build_header(const std::vector<scores::score> &scores) {
    std::stringstream result;

    result << helper::fix_beatmap_status(this->ranked_status) << "|false|" << this->beatmap_id << "|" << this->beatmapset_id << "|" << scores.size() << std::endl;
    result << "0" << std::endl;
    result << this->song_name << std::endl;
    result << "10.0" << std::endl;

    return result.str();
}
