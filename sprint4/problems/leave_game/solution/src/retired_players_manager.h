// retired_players_manager.h - сделаем как в образце
#pragma once

#include <pqxx/pqxx>
#include <string>
#include <boost/json.hpp>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace database
{

    class ConnectionPool
    {
        using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    public:
        class ConnectionWrapper
        {
        public:
            ConnectionWrapper(ConnectionPtr conn, ConnectionPool &pool) noexcept
                : conn_(std::move(conn)), pool_(&pool)
            {
            }

            ConnectionWrapper(const ConnectionWrapper &) = delete;
            ConnectionWrapper &operator=(const ConnectionWrapper &) = delete;
            ConnectionWrapper(ConnectionWrapper &&) = default;
            ConnectionWrapper &operator=(ConnectionWrapper &&) = default;

            pqxx::connection &operator*() const noexcept { return *conn_; }
            pqxx::connection *operator->() const noexcept { return conn_.get(); }

            ~ConnectionWrapper()
            {
                if (conn_)
                    pool_->ReturnConnection(std::move(conn_));
            }

        private:
            ConnectionPtr conn_;
            ConnectionPool *pool_;
        };

        template <typename ConnectionFactory>
        explicit ConnectionPool(size_t capacity, ConnectionFactory factory)
        {
            pool_.reserve(capacity);
            for (size_t i = 0; i < capacity; ++i)
            {
                pool_.push_back(factory());
            }
        }

        ConnectionWrapper GetConnection()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this]
                       { return used_ < pool_.size(); });
            auto conn = std::move(pool_[used_++]);
            return ConnectionWrapper(std::move(conn), *this);
        }

    private:
        void ReturnConnection(ConnectionPtr conn)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                assert(used_ > 0);
                pool_[--used_] = std::move(conn);
            }
            cond_.notify_one();
        }

        std::mutex mutex_;
        std::condition_variable cond_;
        std::vector<ConnectionPtr> pool_;
        size_t used_ = 0;
    };

    void InitializeDB(std::shared_ptr<ConnectionPool> pool);

    struct StatInfo
    {
        std::string name;
        int score;
        double playtime;
    };

    class StatProvider
    {
    public:
        explicit StatProvider(std::shared_ptr<ConnectionPool> pool)
            : pool_(std::move(pool))
        {
        }

        std::vector<StatInfo> GetStats(int start_id, int max_items) const;

    private:
        std::shared_ptr<ConnectionPool> pool_;
    };

} // namespace database