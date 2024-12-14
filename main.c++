#include <iostream>
#include <pqxx/pqxx>
#include <dpp/dpp.h>
#include <ctime>
#include <string>

const std::string TOKEN = "ВАШ_ТОКЕН";
const std::string DATABASE_URL = "URL базы данных(PostgreSQL) или db params если используете pgAdmin4";
const uint64_t REPORT_CHANNEL_ID = 1234567890; // айди канала где будут отчеты

std::vector<uint64_t> allowed_role_ids = {1030078087418884106, 1259291837638901783, 123456789, 123456789, 123456789, 123456789}; // айди ролей которые смогут добавлять, удалять участников.

void execute_query(const std::string& query, const std::vector<std::string>& params) {
    try {
        pqxx::connection conn(DATABASE_URL);
        pqxx::work txn(conn);
        txn.exec_params(query, params);
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    }
}

auto bot = dpp::cluster(TOKEN);

void handle_message(const dpp::message_create_t& event) {
    if (event.msg.author.is_bot) return;
    std::string query;
    std::vector<std::string> params = {std::to_string(event.msg.author.id)};

    try {
        pqxx::connection conn(DATABASE_URL);
        pqxx::work txn(conn);
        pqxx::result res = txn.exec_params("SELECT role_id FROM moderators WHERE user_id = $1", params);

        if (!res.empty()) {
            if (event.msg.content.starts_with("?мьют")) { // можете изменить ключевое слово вместо ?мьют
                query = "INSERT INTO daily_stats (user_id, mutes, date) VALUES ($1, 1, CURRENT_DATE) ON CONFLICT (user_id, date) DO UPDATE SET mutes = daily_stats.mutes + 1";
                execute_query(query, params);
            } else if (event.msg.content.starts_with("?бан")) {  // можете изменить ключевое слово вместо ?бан
                query = "INSERT INTO daily_stats (user_id, bans, date) VALUES ($1, 1, CURRENT_DATE) ON CONFLICT (user_id, date) DO UPDATE SET bans = daily_stats.bans + 1";
                execute_query(query, params);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    }
}
// добавить участника
void add_moderator(const dpp::slashcommand_t& event, uint64_t member_id, uint64_t role_id) {
    if (std::any_of(event.command.usr.roles.begin(), event.command.usr.roles.end(), [&](uint64_t id) {
            return std::find(allowed_role_ids.begin(), allowed_role_ids.end(), id) != allowed_role_ids.end();
        })) {
        std::string query = "INSERT INTO moderators (user_id, role_id) VALUES ($1, $2) ON CONFLICT DO NOTHING";
        execute_query(query, {std::to_string(member_id), std::to_string(role_id)});
        event.reply("✅ Модератор добавлен.");
    } else {
        event.reply("❌ У вас нет прав для выполнения этой команды.");
    }
}
// удалить участника
void remove_moderator(const dpp::slashcommand_t& event, uint64_t member_id) {
    std::string query = "DELETE FROM moderators WHERE user_id = $1";
    execute_query(query, {std::to_string(member_id)});
    event.reply("✅ Модератор удалён.");
}
// отчет
void send_daily_report() {
    try {
        pqxx::connection conn(DATABASE_URL);
        pqxx::work txn(conn);
        pqxx::result stats = txn.exec("SELECT user_id, COALESCE(mutes, 0), COALESCE(bans, 0), role_id FROM moderators LEFT JOIN daily_stats ON moderators.user_id = daily_stats.user_id AND date = CURRENT_DATE");
        std::string report = "📊 Отчет за сегодня:\n";
        for (const auto& row : stats) {
            report += "Пользователь: " + row[0].c_str() + ", Мютов: " + row[1].c_str() + ", Банов: " + row[2].c_str() + "\n";
        }
        bot.message_create(dpp::message(REPORT_CHANNEL_ID, report));
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    }
}

int main() {
    bot.on_log(dpp::utility::cout_logger());
    bot.on_message_create(handle_message);
    bot.on_ready([&](const dpp::ready_t& event) {
        bot.log(dpp::ll_info, "Bot is running");
    });
    bot.start(false);
    return 0;
}
