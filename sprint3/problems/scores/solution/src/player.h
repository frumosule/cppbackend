#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
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

    // Структура для предмета в рюкзаке
    struct BagItem
    {
        LostObject::Id id;
        int type;
        int value; // НОВОЕ: стоимость предмета

        BagItem(LostObject::Id id, int type, int value = 0)
            : id(id), type(type), value(value) {}

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
            : id_(id), name_(std::move(name)), auth_token_(std::move(auth_token)), // ИСПРАВЛЕНО: убрал лишнее std::string
              map_id_(std::move(map_id)), pos_x_(start_x), pos_y_(start_y),
              speed_x_(0.0), speed_y_(0.0), direction_(Direction::NORTH),
              bag_capacity_(bag_capacity), score_(0)
        {
        } // НОВОЕ: инициализируем score

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

        // Функции для работы с рюкзаком
        const std::vector<BagItem> &GetBag() const noexcept { return bag_; }
        size_t GetBagCapacity() const noexcept { return bag_capacity_; }
        size_t GetBagSize() const noexcept { return bag_.size(); }
        bool IsBagFull() const noexcept { return bag_.size() >= bag_capacity_; }

        void SetBagCapacity(size_t capacity) { bag_capacity_ = capacity; }

        // ИСПРАВЛЕНО: добавил параметр value
        bool AddToBag(LostObject::Id id, int type, int value = 0)
        {
            if (IsBagFull())
            {
                return false;
            }
            // Проверяем, нет ли уже такого предмета в рюкзаке
            for (const auto &item : bag_)
            {
                if (item.id == id)
                {
                    return false; // Предмет уже в рюкзаке
                }
            }
            bag_.emplace_back(id, type, value); // ИСПРАВЛЕНО: передаем value
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

        // Удаление конкретного предмета из рюкзака (опционально)
        bool RemoveFromBag(LostObject::Id id)
        {
            for (auto it = bag_.begin(); it != bag_.end(); ++it)
            {
                if (it->id == id)
                {
                    bag_.erase(it);
                    return true;
                }
            }
            return false;
        }

        // НОВОЕ: Методы для работы с очками
        int GetScore() const noexcept { return score_; }
        void AddScore(int points) { score_ += points; }
        void SetScore(int score) { score_ = score; }

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

        // Рюкзак
        std::vector<BagItem> bag_;
        size_t bag_capacity_;
        int score_ = 0; // НОВОЕ: счет игрока
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