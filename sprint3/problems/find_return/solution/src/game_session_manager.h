#pragma once

#include "model.h"
#include "player.h"
#include "loot_generator.h"
#include <random>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <forward_list>

namespace model {

class GameSessionManager {
public:
    explicit GameSessionManager(Game& game) : game_(game) {}
    
    std::shared_ptr<GameSession> FindSession(const Map::Id& map_id);
    std::shared_ptr<Player> AddPlayer(const Map::Id& map_id, const std::string& player_name);
    std::shared_ptr<Player> FindPlayerByToken(const std::string& auth_token);
    std::shared_ptr<Player> FindPlayerById(Player::Id player_id);
    bool UpdatePlayerAction(const std::string& auth_token, const std::string& move);
    std::unordered_map<Player::Id, std::shared_ptr<Player>> GetPlayersByToken(const std::string& auth_token);
    void UpdateGameState(int time_delta_ms);
    void SetRandomSpawn(bool randomize) { randomize_spawn_ = randomize; }
    bool IsRandomSpawn() const { return randomize_spawn_; }
    
    // НОВОЕ: Инициализация генератора лута
    void InitializeLootGenerator(double base_interval, double probability);
    const std::unordered_map<LostObject::Id, LostObject>& GetLostObjects(const Map::Id& map_id) const;

private:
    Game& game_;

    // ИСПРАВЛЕНО: Структура данных сессии с лутом
    struct SessionData {
        std::unordered_map<Player::Id, std::shared_ptr<Player>> players;
        std::unordered_map<LostObject::Id, LostObject> lost_objects;
        LostObject::Id next_lost_object_id = 0;
        
        // Конструктор для SessionData
        SessionData() = default;
    };

    // ИСПРАВЛЕНО: Используем SessionData вместо GameSession
    std::unordered_map<Map::Id, std::shared_ptr<SessionData>> sessions_;
    std::unordered_map<std::string, std::shared_ptr<Player>> players_by_token_;
    std::unordered_map<Player::Id, std::shared_ptr<Player>> players_by_id_;
    mutable std::mutex mutex_;  // ИСПРАВЛЕНО: mutable для const методов
    Player::Id next_player_id_ = 0;
    std::mt19937 generator_{std::random_device{}()};  // ИСПРАВЛЕНО: mt19937 вместо mt19937_64
    bool randomize_spawn_ = false;
    
    // Генерация токенов
    std::random_device random_device_;  
    std::mt19937_64 generator1_{random_device_()};
    std::mt19937_64 generator2_{random_device_()};
    
    // НОВОЕ: Генератор лута
    std::unique_ptr<loot_gen::LootGenerator> loot_generator_;
    
    std::string GenerateAuthToken();
    std::pair<double, double> GetRandomPositionOnRoad(const Map::Id& map_id);
    
    // ИСПРАВЛЕНО: Методы для работы с SessionData
    std::shared_ptr<SessionData> FindSessionUnsafe(const Map::Id& map_id);
    std::shared_ptr<SessionData> CreateSessionUnsafe(const Map::Id& map_id);
    std::shared_ptr<Player> FindPlayerByTokenUnsafe(const std::string& auth_token);
    
    // НОВОЕ: Методы для работы с лутом
    void GenerateLootForSession(SessionData& session, const Map::Id& map_id, int time_delta_ms);
    size_t GetBagCapacityForMap(const Map::Id &map_id) const;
    
    // Логика движения как в образце
    struct PointHash {
        size_t operator()(const Point& p) const {
            return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
        }
    };
    
    std::unordered_map<Point, std::vector<const Road*>, model::PointHash> BuildRoadsGraph(const Map* map);
    bool InBounds(const Road* road, double x, double y) const;
    double GetMaxPossible(const Road* road, const std::string& direction) const;
    std::pair<bool, std::pair<double, double>> CalculateMove(double x, double y, double speed_x, double speed_y, int delta, 
                                                           const std::unordered_map<Point, std::vector<const Road*>, model::PointHash>& roads_graph) const;
};

} // namespace model