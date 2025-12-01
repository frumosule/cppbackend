#pragma once

#include "model.h"
#include "player.h"
#include "application_listener.h"
#include "loot_generator.h"
#include <random>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <forward_list>
#include "retired_players_manager.h"

namespace model
{

    class GameSessionManager
    {
    public:
        explicit GameSessionManager(Game &game) : game_(game) {}

        std::shared_ptr<GameSession> FindSession(const Map::Id &map_id);
        std::shared_ptr<Player> AddPlayer(const Map::Id &map_id, const std::string &player_name);
        std::shared_ptr<Player> FindPlayerByToken(const std::string &auth_token);
        std::shared_ptr<Player> FindPlayerById(Player::Id player_id);
        bool UpdatePlayerAction(const std::string &auth_token, const std::string &move);
        std::unordered_map<Player::Id, std::shared_ptr<Player>> GetPlayersByToken(const std::string &auth_token);
        void UpdateGameState(int time_delta_ms);
        void SetRandomSpawn(bool randomize) { randomize_spawn_ = randomize; }
        bool IsRandomSpawn() const { return randomize_spawn_; }

        void InitializeLootGenerator(double base_interval, double probability);
        const std::unordered_map<LostObject::Id, LostObject> &GetLostObjects(const Map::Id &map_id) const;
        void SetLootValuesForMap(const Map::Id &map_id, std::unordered_map<int, int> loot_values);
        std::unordered_map<int, int> GetLootValuesForMap(const Map::Id &map_id) const;

        void AddListener(std::shared_ptr<app::ApplicationListener> listener);
        void NotifyListeners(std::chrono::milliseconds delta);
        void NotifyShutdown();

        void Serialize(std::ostream &out) const;
        void Deserialize(std::istream &in);

        void CheckInactivePlayers();
        void RetirePlayer(Player::Id player_id);
        void SetRetirementTimeout(std::chrono::milliseconds timeout)
        {
            retirement_timeout_ = timeout;
        }

        void SetStatProvider(std::shared_ptr<database::StatProvider> provider)
        {
            stat_provider_ = provider;
        }

    private:
        Game &game_;

        struct SessionData
        {
            std::unordered_map<Player::Id, std::shared_ptr<Player>> players;
            std::unordered_map<LostObject::Id, LostObject> lost_objects;
            LostObject::Id next_lost_object_id = 0;

            SessionData() = default;
        };

        struct PlayerSessionData {
            std::chrono::steady_clock::time_point join_time;
            std::chrono::steady_clock::time_point last_movement_time;
            std::chrono::steady_clock::time_point inactive_start_time;
            bool is_active = true;
            bool inactive_started = false;
            unsigned total_playtime_ms = 0;
        };

        std::unordered_map<Map::Id, std::shared_ptr<SessionData>> sessions_;
        std::unordered_map<std::string, std::shared_ptr<Player>> players_by_token_;
        std::unordered_map<Player::Id, std::shared_ptr<Player>> players_by_id_;
        std::unordered_map<Player::Id, PlayerSessionData> player_sessions_;
        mutable std::mutex mutex_;
        Player::Id next_player_id_ = 0;
        std::mt19937 generator_{std::random_device{}()};
        bool randomize_spawn_ = false;
        std::chrono::milliseconds retirement_timeout_ = std::chrono::seconds(60); // по умолчанию 1 минута
        std::shared_ptr<database::StatProvider> stat_provider_;

        std::vector<std::shared_ptr<app::ApplicationListener>> listeners_;
        mutable std::mutex listeners_mutex_;

        std::random_device random_device_;
        std::mt19937_64 generator1_{random_device_()};
        std::mt19937_64 generator2_{random_device_()};

        std::unique_ptr<loot_gen::LootGenerator> loot_generator_;
        std::unordered_map<Map::Id, std::unordered_map<int, int>, Game::MapIdHasher> map_loot_values_;

        std::shared_ptr<RetiredPlayersManager> retired_players_manager_;

        std::string GenerateAuthToken();
        std::pair<double, double> GetRandomPositionOnRoad(const Map::Id &map_id);

        std::shared_ptr<SessionData> FindSessionUnsafe(const Map::Id &map_id);
        std::shared_ptr<SessionData> CreateSessionUnsafe(const Map::Id &map_id);
        std::shared_ptr<Player> FindPlayerByTokenUnsafe(const std::string &auth_token);
        void CheckInactivePlayersUnsafe();
        void RetirePlayerUnsafe(Player::Id player_id);

        void GenerateLootForSession(SessionData &session, const Map::Id &map_id, int time_delta_ms);
        size_t GetBagCapacityForMap(const Map::Id &map_id) const;

        struct PointHash
        {
            size_t operator()(const Point &p) const
            {
                return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
            }
        };

        std::unordered_map<Point, std::vector<const Road *>, model::PointHash> BuildRoadsGraph(const Map *map);
        bool InBounds(const Road *road, double x, double y) const;
        double GetMaxPossible(const Road *road, const std::string &direction) const;
        std::pair<bool, std::pair<double, double>> CalculateMove(double x, double y, double speed_x, double speed_y, int delta,
                                                                 const std::unordered_map<Point, std::vector<const Road *>, model::PointHash> &roads_graph) const;

        void RemovePlayerUnsafe(Player::Id player_id);
    };

} // namespace model