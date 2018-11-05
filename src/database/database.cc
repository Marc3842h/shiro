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

#include "../config/cli_args.hh"
#include "../thirdparty/loguru.hh"
#include "../shiro.hh"
#include "database.hh"
#include "tables/channel_table.hh"

shiro::database::database(const std::string &address, uint32_t port, const std::string &db, const std::string &username, const std::string &password)
    : address(address), port(port), db(db), username(username), password(password) {
    auto [argc, argv] = config::cli::get_args();
    sqlpp::mysql::global_library_init(argc, argv);
}

void shiro::database::connect() {
    if (this->is_connected())
        return;

    this->config = std::make_shared<sqlpp::mysql::connection_config>();
    this->config->host = this->address;
    this->config->port = this->port;
    this->config->database = this->db;
    this->config->user = this->username;
    this->config->password = this->password;
    this->config->auto_reconnect = true;

#if defined(_DEBUG) && !defined(SHIRO_NO_SQL_DEBUG)
    this->config->debug = true;
#endif

    LOG_S(INFO) << "Successfully connected to MySQL database.";
}

void shiro::database::setup() {
    if (!this->is_connected(true))
        return;

    sqlpp::mysql::connection db(this->config);

    // Beatmaps
    db.execute(
        "CREATE TABLE IF NOT EXISTS `beatmaps` "
        "(id INT PRIMARY KEY NOT NULL AUTO_INCREMENT, "
        "beatmap_id INT NOT NULL, beatmapset_id INT NOT NULL, beatmap_md5 VARCHAR(35) NOT NULL, song_name VARCHAR(128) NOT NULL, "
        "game_mode SMALLINT NOT NULL, ar FLOAT NOT NULL, od FLOAT NOT NULL, diff_std FLOAT NOT NULL, diff_taiko FLOAT NOT NULL, diff_ctb FLOAT NOT NULL, diff_mania FLOAT NOT NULL, "
        "max_combo INT NOT NULL, hit_length INT NOT NULL, bpm INT NOT NULL, ranked_status INT NOT NULL, last_update INT NOT NULL, "
        "ranked_status_freezed BOOLEAN NOT NULL, play_count INT NOT NULL, pass_count INT NOT NULL);");

    // Channels
    db.execute(
        "CREATE TABLE IF NOT EXISTS `channels` "
        "(id INT PRIMARY KEY NOT NULL AUTO_INCREMENT, "
        "name VARCHAR(32) NOT NULL, description VARCHAR(64) NOT NULL, "
        "auto_join BOOLEAN NOT NULL, hidden BOOLEAN NOT NULL);");

    // Submitted scores
    db.execute(
        "CREATE TABLE IF NOT EXISTS `scores` "
        "(id INT PRIMARY KEY NOT NULL AUTO_INCREMENT, "
        "beatmap_md5 VARCHAR(35) NOT NULL, hash VARCHAR(35) NOT NULL, user_id INT NOT NULL, rank VARCHAR(2) NOT NULL, "
        "score BIGINT NOT NULL, max_combo INT NOT NULL, fc BOOLEAN NOT NULL, mods INT NOT NULL, "
        "300_count INT NOT NULL, 100_count INT NOT NULL, 50_count INT NOT NULL, "
        "katus_count INT NOT NULL, gekis_count INT NOT NULL, miss_count INT NOT NULL, "
        "time INT NOT NULL, play_mode TINYINT NOT NULL, passed BOOLEAN NOT NULL, "
        "accuracy FLOAT NOT NULL, pp FLOAT NOT NULL, times_watched INT NOT NULL DEFAULT 0);");

    // Registered users and their stats
    db.execute(
        "CREATE TABLE IF NOT EXISTS `users` "
        "(id INT PRIMARY KEY NOT NULL AUTO_INCREMENT, "
        "username VARCHAR(32) NOT NULL, safe_username VARCHAR(32) NOT NULL, "
        "password VARCHAR(128) NOT NULL, salt VARCHAR(64) NOT NULL,"
        "email VARCHAR(100) NOT NULL, ip VARCHAR(39) NOT NULL, registration_date INT NOT NULL, last_seen INT NOT NULL, "
        "followers INT NOT NULL, roles INT UNSIGNED NOT NULL, user_page TEXT NOT NULL, "
        "pp_std FLOAT NOT NULL DEFAULT 0, pp_taiko FLOAT NOT NULL DEFAULT 0, pp_ctb FLOAT NOT NULL DEFAULT 0, "
        "pp_mania FLOAT NOT NULL DEFAULT 0, rank_std INT NOT NULL DEFAULT 0, rank_taiko INT NOT NULL DEFAULT 0, "
        "rank_ctb INT NOT NULL DEFAULT 0, rank_mania INT NOT NULL DEFAULT 0, score_std INT NOT NULL DEFAULT 0, "
        "score_taiko INT NOT NULL DEFAULT 0, score_ctb INT NOT NULL DEFAULT 0, score_mania INT NOT NULL DEFAULT 0, "
        "ranked_score_std INT NOT NULL DEFAULT 0, ranked_score_taiko INT NOT NULL DEFAULT 0, "
        "ranked_score_ctb INT NOT NULL DEFAULT 0, ranked_score_mania INT NOT NULL DEFAULT 0, "
        "accuracy_std FLOAT NOT NULL DEFAULT 0, accuracy_taiko FLOAT NOT NULL DEFAULT 0, accuracy_ctb FLOAT NOT NULL DEFAULT 0, "
        "accuracy_mania FLOAT NOT NULL DEFAULT 0, play_count_std INT NOT NULL DEFAULT 0, play_count_taiko INT NOT NULL DEFAULT 0, "
        "play_count_ctb INT NOT NULL DEFAULT 0, play_count_mania INT NOT NULL DEFAULT 0, country VARCHAR(2) NOT NULL);");

    // Punishments (kicks, silences, restrictions, bans)
    db.execute(
        "CREATE TABLE IF NOT EXISTS `punishments` "
        "(id INT PRIMARY KEY NOT NULL AUTO_INCREMENT, user_id INT NOT NULL, "
        "type TINYINT UNSIGNED NOT NULL, time INT NOT NULL, duration INT DEFAULT NULL, "
        "active BOOLEAN NOT NULL, reason VARCHAR(128) DEFAULT NULL);");

    // Relationships between users (friends and blocked)
    db.execute(
        "CREATE TABLE IF NOT EXISTS `relationships` (origin INT NOT NULL, target INT NOT NULL, blocked BOOLEAN NOT NULL);");

    // Roles
    db.execute(
        "CREATE TABLE IF NOT EXISTS `roles` "
        "(id INT UNSIGNED PRIMARY KEY NOT NULL, name VARCHAR(32) NOT NULL, "
        "permissions BIGINT UNSIGNED NOT NULL, color TINYINT UNSIGNED NOT NULL);");

    LOG_S(INFO) << "Successfully created and structured tables in database.";
}

bool shiro::database::is_connected(bool abort) {
    if (this->config == nullptr)
        return false;

    try {
        sqlpp::mysql::connection db(this->config);

        return db.is_valid();
    } catch (const sqlpp::exception &ex) {
        if (abort)
            LOG_S(FATAL) << "Unable to connect to database: " << ex.what();

        return false;
    }

    return true;
}

std::shared_ptr<sqlpp::mysql::connection_config> shiro::database::get_config() {
    return this->config;
}
