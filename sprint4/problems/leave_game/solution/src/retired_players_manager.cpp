#include "retired_players_manager.h"
#include <stdexcept>

RetiredPlayersManager::RetiredPlayersManager(DatabaseConnectionPool &pool)
    : connection_pool_(pool) {}

void RetiredPlayersManager::AddRetiredPlayer(const std::string &name, int score, long play_time_ms)
{
    auto conn_wrapper = connection_pool_.GetConnection();
    pqxx::work w(*conn_wrapper);

    w.exec_prepared("add_retired_player", name, score, play_time_ms);
    w.commit();
}

boost::json::array RetiredPlayersManager::GetRecords(int start, int max_items)
{
    if (max_items > 100)
    {
        throw std::invalid_argument("maxItems cannot exceed 100");
    }

    auto conn_wrapper = connection_pool_.GetConnection();
    pqxx::read_transaction r(*conn_wrapper);

    auto result = r.exec_prepared("get_records", start, max_items);

    boost::json::array records;
    for (const auto &row : result)
    {
        boost::json::object record;
        record["name"] = row["name"].as<std::string>();
        record["score"] = row["score"].as<int>();
        record["playTime"] = row["play_time_ms"].as<long>() / 1000.0;
        records.push_back(record);
    }

    return records;
}