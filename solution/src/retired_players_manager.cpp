#include "retired_players_manager.h"
#include <iostream>

namespace database
{

    void InitializeDB(std::shared_ptr<ConnectionPool> pool)
    {
        auto conn = pool->GetConnection();
        pqxx::work work{*conn};

        std::cout << "Initializing database..." << std::endl;

        work.exec(R"(
CREATE TABLE IF NOT EXISTS retired_players (
    id SERIAL PRIMARY KEY,
    name varchar(100) NOT NULL,
    score int NOT NULL,
    playtime double precision NOT NULL
);
)");

        work.exec(R"(
CREATE INDEX IF NOT EXISTS retired_players_score_idx ON retired_players (score DESC);
)");
        work.exec(R"(
CREATE INDEX IF NOT EXISTS retired_players_playtime_idx ON retired_players (playtime ASC);
)");
        work.exec(R"(
CREATE INDEX IF NOT EXISTS retired_players_name_idx ON retired_players (name ASC);
)");

        conn->prepare("add_retired_player",
                      "INSERT INTO retired_players (name, score, playtime) VALUES ($1, $2, $3)");
        conn->prepare("get_records",
                      "SELECT name, score, playtime FROM retired_players "
                      "ORDER BY score DESC, playtime ASC, name ASC "
                      "LIMIT $2 OFFSET $1");

        work.commit();
        std::cout << "Database initialized successfully" << std::endl;
    }

    std::vector<StatInfo> StatProvider::GetStats(int start, int max_items) const
    {
        if (!pool_)
        {
            std::cerr << "StatProvider::GetStats: pool is null" << std::endl;
            return {};
        }

        try
        {
            auto conn = pool_->GetConnection();
            pqxx::work work{*conn};
            std::vector<StatInfo> stats;

            auto result = work.exec_prepared("get_records", start, max_items);

            std::cout << "Got " << result.size() << " records from database" << std::endl;

            for (const auto &row : result)
            {
                try
                {
                    auto [name, score, playtime] = row.as<std::string, int, double>();
                    stats.push_back({name, score, playtime});
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error parsing row: " << e.what() << std::endl;
                }
            }

            work.commit();
            return stats;
        }
        catch (const std::exception &e)
        {
            std::cerr << "StatProvider::GetStats error: " << e.what() << std::endl;
            return {};
        }
    }

    void StatProvider::AddRetiredPlayer(const std::string &name, int score, double playtime_seconds)
    {
        if (!pool_)
        {
            std::cerr << "StatProvider::AddRetiredPlayer: pool is null" << std::endl;
            return;
        }

        try
        {
            auto conn = pool_->GetConnection();
            pqxx::work w(*conn);

            std::cout << "Adding retired player: " << name
                      << ", score: " << score
                      << ", playtime: " << playtime_seconds << "s" << std::endl;

            w.exec_prepared("add_retired_player", name, score, playtime_seconds);
            w.commit();

            std::cout << "Retired player added successfully" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error adding retired player: " << e.what() << std::endl;
        }
    }

} // namespace database