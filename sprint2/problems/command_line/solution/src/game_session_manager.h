#pragma once

#include "model.h"
#include "player.h"
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

private:
    Game& game_;
    std::unordered_map<Map::Id, std::shared_ptr<GameSession>> sessions_;
    std::unordered_map<std::string, std::shared_ptr<Player>> players_by_token_;
    std::unordered_map<Player::Id, std::shared_ptr<Player>> players_by_id_;
    std::mutex mutex_;
    Player::Id next_player_id_ = 0;
    std::mt19937_64 generator_{std::random_device{}()};
    bool randomize_spawn_ = false;
    
    // Генерация токенов
    std::random_device random_device_;  
    std::mt19937_64 generator1_{random_device_()};
    std::mt19937_64 generator2_{random_device_()};
    std::uniform_int_distribution<uint64_t> distribution_;
    
    std::string GenerateAuthToken();
    std::pair<double, double> GetRandomPositionOnRoad(const Map::Id& map_id);
    std::shared_ptr<GameSession> FindSessionUnsafe(const Map::Id& map_id);
    std::shared_ptr<GameSession> CreateSessionUnsafe(const Map::Id& map_id);
    std::shared_ptr<Player> FindPlayerByTokenUnsafe(const std::string& auth_token);
    
    // Новая логика движения как в образце
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
