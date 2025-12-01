// retired_players_manager.cpp - сделаем как в образце
#include "retired_players_manager.h"

namespace database {

void InitializeDB(std::shared_ptr<ConnectionPool> pool) {
    auto conn = pool->GetConnection();
    pqxx::work work{ *conn };
    work.exec(R"(
CREATE TABLE IF NOT EXISTS retired_players (
    id SERIAL PRIMARY KEY,
    name varchar(100) NOT NULL,
    score int NOT NULL,
    playtime double precision NOT NULL
);
)");
    work.exec(R"(
CREATE INDEX IF NOT EXISTS score_idx ON retired_players (score);
)");
    work.exec(R"(
CREATE INDEX IF NOT EXISTS playtime_idx ON retired_players (playtime);
)");
    work.exec(R"(
CREATE INDEX IF NOT EXISTS name_idx ON retired_players (name);
)");
    
    // Подготовленные запросы
    conn->prepare("add_retired_player", 
        "INSERT INTO retired_players (name, score, playtime) VALUES ($1, $2, $3)");
    conn->prepare("get_records", 
        "SELECT name, score, playtime FROM retired_players "
        "ORDER BY score DESC, playtime ASC, name ASC "
        "LIMIT $2 OFFSET $1");
        
    work.commit();
}

std::vector<StatInfo> StatProvider::GetStats(int start, int max_items) const {
    auto conn = pool_->GetConnection();
    pqxx::work work{ *conn };
    std::vector<StatInfo> stats;
    
    auto result = work.exec_prepared("get_records", start, max_items);
    
    for (const auto& row : result) {
        auto [name, score, playtime] = row.as<std::string, int, double>();
        stats.push_back({name, score, playtime});
    }
    
    return stats;
}

// Для совместимости со старым кодом
RetiredPlayersManager::RetiredPlayersManager(ConnectionPool& pool)
    : pool_(pool) {
    // Инициализация уже сделана в InitializeDB
}

void RetiredPlayersManager::AddRetiredPlayer(const std::string& name, int score, long play_time_ms) {
    auto conn = pool_.GetConnection();
    pqxx::work w(*conn);
    
    double playtime_seconds = play_time_ms / 1000.0;
    w.exec_prepared("add_retired_player", name, score, playtime_seconds);
    w.commit();
}

boost::json::array RetiredPlayersManager::GetRecords(int start, int max_items) {
    if (max_items > 100) {
        throw std::invalid_argument("maxItems cannot exceed 100");
    }
    
    StatProvider provider(std::make_shared<ConnectionPool>(1, 
        [this]() { 
            // Создаем новое соединение
            return std::make_shared<pqxx::connection>(/* connection string */); 
        }));
    
    auto stats = provider.GetStats(start, max_items);
    
    boost::json::array records;
    for (const auto& stat : stats) {
        boost::json::object record;
        record["name"] = stat.name;
        record["score"] = stat.score;
        record["playTime"] = stat.playtime;  // уже в секундах как double
        records.push_back(record);
    }
    
    return records;
}

} // namespace database