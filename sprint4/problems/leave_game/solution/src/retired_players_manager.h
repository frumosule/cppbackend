#pragma once

#include <pqxx/pqxx>
#include <string>
#include <unordered_map>
#include <boost/json.hpp>
#include <memory>
#include <mutex>
#include <condition_variable>

class DatabaseConnectionPool {
private:
    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<std::shared_ptr<pqxx::connection>> pool_;
    size_t used_connections_ = 0;
    std::string connection_string_;

public:
    class ConnectionWrapper {
    private:
        std::shared_ptr<pqxx::connection> conn_;
        DatabaseConnectionPool* pool_;

    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, DatabaseConnectionPool* pool) 
            : conn_(std::move(conn)), pool_(pool) {}
        
        ~ConnectionWrapper() {
            if (conn_ && pool_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

        pqxx::connection& operator*() { return *conn_; }
        pqxx::connection* operator->() { return conn_.get(); }
        std::shared_ptr<pqxx::connection> get() { return conn_; }
    };

    DatabaseConnectionPool(size_t capacity, const std::string& conn_str) 
        : connection_string_(conn_str) {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            auto conn = std::make_shared<pqxx::connection>(conn_str);
            InitializeConnection(*conn);
            pool_.push_back(conn);
        }
    }

    ConnectionWrapper GetConnection() {
        std::unique_lock lock(mutex_);
        cond_var_.wait(lock, [this] { return used_connections_ < pool_.size(); });
        return { std::move(pool_[used_connections_++]), this };
    }

private:
    void ReturnConnection(std::shared_ptr<pqxx::connection>&& conn) {
        {
            std::lock_guard lock(mutex_);
            pool_[--used_connections_] = std::move(conn);
        }
        cond_var_.notify_one();
    }

    void InitializeConnection(pqxx::connection& conn) {
        pqxx::work w(conn);
        w.exec(R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
                name VARCHAR(100) NOT NULL,
                score INTEGER NOT NULL,
                play_time_ms BIGINT NOT NULL,
                retirement_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
            
            CREATE INDEX IF NOT EXISTS idx_retired_players_score_time_name 
            ON retired_players (score DESC, play_time_ms ASC, name ASC);
        )"_zv);
        
        // Подготавливаем запросы
        conn.prepare("add_retired_player", 
            "INSERT INTO retired_players (name, score, play_time_ms) VALUES ($1, $2, $3)");
        
        conn.prepare("get_records", 
            "SELECT name, score, play_time_ms FROM retired_players "
            "ORDER BY score DESC, play_time_ms ASC, name ASC "
            "LIMIT $2 OFFSET $1");
            
        w.commit();
    }
};

class RetiredPlayersManager {
public:
    explicit RetiredPlayersManager(DatabaseConnectionPool& pool);
    
    void AddRetiredPlayer(const std::string& name, int score, long play_time_ms);
    
    boost::json::array GetRecords(int start = 0, int max_items = 100);
    
private:
    DatabaseConnectionPool& connection_pool_;
};