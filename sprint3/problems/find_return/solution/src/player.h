#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "model.h"

namespace model
{

    enum class Direction
    {
        NORTH = 'U',
        SOUTH = 'D',
        WEST = 'L',
        EAST = 'R'
    };

    struct BagItem
    {
        LostObject::Id id;
        int type;

        BagItem(LostObject::Id id, int type) : id(id), type(type) {}

        // Для удобства сравнения
        bool operator==(const BagItem &other) const
        {
            return id == other.id && type == other.type;
        }
    };

    class Player
    {
    public:
        using Id = size_t;

        Player(Id id, std::string name, std::string auth_token, Map::Id map_id, double start_x, double start_y, size_t bag_capacity = 3)
            : id_(id), name_(std::move(name)), auth_token_(std::move(auth_token)),
              map_id_(std::move(map_id)), pos_x_(start_x), pos_y_(start_y),
              speed_x_(0.0), speed_y_(0.0), direction_(Direction::NORTH),
              bag_capacity_(bag_capacity) {}

        Id GetId() const noexcept { return id_; }
        const std::string &GetName() const noexcept { return name_; }
        const std::string &GetAuthToken() const noexcept { return auth_token_; }
        const Map::Id &GetMapId() const noexcept { return map_id_; }

        double GetPosX() const noexcept { return pos_x_; }
        double GetPosY() const noexcept { return pos_y_; }
        void SetPosition(double x, double y)
        {
            pos_x_ = x;
            pos_y_ = y;
        }

        double GetSpeedX() const noexcept { return speed_x_; }
        double GetSpeedY() const noexcept { return speed_y_; }
        void SetSpeed(double speed_x, double speed_y)
        {
            speed_x_ = speed_x;
            speed_y_ = speed_y;
        }

        Direction GetDirection() const noexcept { return direction_; }
        void SetDirection(Direction dir) { direction_ = dir; }

        char GetDirectionChar() const noexcept
        {
            return static_cast<char>(direction_);
        }

        const std::vector<BagItem> &GetBag() const noexcept { return bag_; }
        size_t GetBagCapacity() const noexcept { return bag_capacity_; }
        size_t GetBagSize() const noexcept { return bag_.size(); }
        bool IsBagFull() const noexcept { return bag_.size() >= bag_capacity_; }

        void SetBagCapacity(size_t capacity) { bag_capacity_ = capacity; }

        bool AddToBag(LostObject::Id id, int type)
        {
            if (IsBagFull())
            {
                return false;
            }
            bag_.emplace_back(id, type);
            return true;
        }

        void ClearBag()
        {
            bag_.clear();
        }

        bool HasItem(LostObject::Id id) const
        {
            for (const auto &item : bag_)
            {
                if (item.id == id)
                {
                    return true;
                }
            }
            return false;
        }

    private:
        Id id_;
        std::string name_;
        std::string auth_token_;
        Map::Id map_id_;
        double pos_x_;
        double pos_y_;
        double speed_x_;
        double speed_y_;
        Direction direction_;
        std::vector<BagItem> bag_;
        size_t bag_capacity_;
    };

    class GameSession
    {
    public:
        using Id = Map::Id;

        explicit GameSession(Map::Id map_id) : map_id_(std::move(map_id)) {}

        const Map::Id &GetId() const noexcept { return map_id_; }

        void AddPlayer(std::shared_ptr<Player> player)
        {
            players_[player->GetId()] = player;
        }

        void RemovePlayer(Player::Id player_id)
        {
            players_.erase(player_id);
        }

        const std::unordered_map<Player::Id, std::shared_ptr<Player>> &GetPlayers() const noexcept
        {
            return players_;
        }

        std::shared_ptr<Player> FindPlayer(Player::Id player_id) const
        {
            auto it = players_.find(player_id);
            return it != players_.end() ? it->second : nullptr;
        }

        bool HasPlayer(Player::Id player_id) const
        {
            return players_.find(player_id) != players_.end();
        }

    private:
        Map::Id map_id_;
        std::unordered_map<Player::Id, std::shared_ptr<Player>> players_;
    };

} // namespace model