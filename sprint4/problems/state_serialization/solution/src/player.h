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

    struct BagItem
    {
        LostObject::Id id;
        int type;
        int value;

        BagItem() = default;
        BagItem(LostObject::Id id, int type, int value = 0);
        bool operator==(const BagItem &other) const;
    };

    class Player
    {
    public:
        using Id = size_t;

        Player() = default; 
        Player(Id id, std::string name, std::string auth_token, Map::Id map_id,
               double start_x, double start_y, size_t bag_capacity = 3);

        Id GetId() const noexcept;
        const std::string &GetName() const noexcept;
        const std::string &GetAuthToken() const noexcept;
        const Map::Id &GetMapId() const noexcept;

        double GetPosX() const noexcept;
        double GetPosY() const noexcept;
        void SetPosition(double x, double y);

        double GetSpeedX() const noexcept;
        double GetSpeedY() const noexcept;
        void SetSpeed(double speed_x, double speed_y);

        Direction GetDirection() const noexcept;
        void SetDirection(Direction dir);
        char GetDirectionChar() const noexcept;

        const std::vector<BagItem> &GetBag() const noexcept;
        size_t GetBagCapacity() const noexcept;
        size_t GetBagSize() const noexcept;
        bool IsBagFull() const noexcept;
        void SetBagCapacity(size_t capacity);

        bool AddToBag(LostObject::Id id, int type, int value = 0);
        void ClearBag();
        bool HasItem(LostObject::Id id) const;
        bool RemoveFromBag(LostObject::Id id);

        int GetScore() const noexcept;
        void AddScore(int points);
        void SetScore(int score);

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
        int score_ = 0;
    };

    class GameSession
    {
    public:
        using Id = Map::Id;

        explicit GameSession(Map::Id map_id);
        const Map::Id &GetId() const noexcept;

        void AddPlayer(std::shared_ptr<Player> player);
        void RemovePlayer(Player::Id player_id);

        const std::unordered_map<Player::Id, std::shared_ptr<Player>> &GetPlayers() const noexcept;
        std::shared_ptr<Player> FindPlayer(Player::Id player_id) const;
        bool HasPlayer(Player::Id player_id) const;

    private:
        Map::Id map_id_;
        std::unordered_map<Player::Id, std::shared_ptr<Player>> players_;
    };

} // namespace model

