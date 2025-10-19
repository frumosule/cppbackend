#pragma once

#include "model.h"
#include "player.h"
#include <unordered_map>
#include <memory>
#include <random>
#include <mutex>
#include <vector>

namespace model
{

    class GameSessionManager
    {
    public:
        // Добавляем Game в конструктор
        explicit GameSessionManager(Game &game)
            : game_(game),
              next_player_id_(1),
              generator_(std::random_device{}()),
              distribution_(0, std::numeric_limits<uint64_t>::max()) {}

        std::shared_ptr<GameSession> FindSession(const Map::Id &map_id);
        std::shared_ptr<Player> AddPlayer(const Map::Id &map_id, const std::string &player_name);
        std::shared_ptr<Player> FindPlayerByToken(const std::string &auth_token);
        std::shared_ptr<Player> FindPlayerById(Player::Id player_id);
        bool UpdatePlayerAction(const std::string &auth_token, const std::string &move, double dog_speed);
        std::unordered_map<Player::Id, std::shared_ptr<Player>> GetPlayersByToken(const std::string &auth_token);
        void UpdateGameState(int time_delta_ms);

    private:
        Game &game_; // Добавляем ссылку на Game
        std::unordered_map<Map::Id, std::shared_ptr<GameSession>> sessions_;
        std::unordered_map<std::string, std::shared_ptr<Player>> players_by_token_;
        std::unordered_map<Player::Id, std::shared_ptr<Player>> players_by_id_;
        Player::Id next_player_id_;
        std::mt19937_64 generator_;
        std::uniform_int_distribution<uint64_t> distribution_;
        std::mutex mutex_;

        // Вспомогательные методы без блокировки
        std::shared_ptr<GameSession> FindSessionUnsafe(const Map::Id &map_id);
        std::shared_ptr<GameSession> CreateSessionUnsafe(const Map::Id &map_id);
        std::shared_ptr<Player> FindPlayerByTokenUnsafe(const std::string &auth_token);
        std::string GenerateAuthToken();
        std::pair<double, double> GetRandomPositionOnRoad(const Map::Id &map_id);
        
        // Основные методы для работы с дорогами и перемещениями
        bool IsPositionOnRoad(const Road &road, double x, double y);
        std::vector<const Road*> FindConnectedRoads(const std::vector<Road>& roads, double x, double y);
        bool AreRoadsConnected(const Road& road1, const Road& road2);
        std::vector<std::pair<double, double>> GetRoadEndpoints(const Road& road);
        std::pair<double, bool> CalculateMovementWithConnections(const std::vector<const Road*>& connected_roads, 
                                                                double current_x, double current_y,
                                                                double target_x, double target_y, 
                                                                double speed_x, double speed_y, bool is_horizontal);
        
        // Старые методы для обратной совместимости
        const model::Road *FindRoadForPosition(const std::vector<model::Road> &roads, double x, double y);
        std::pair<double, bool> CalculateHorizontalMovement(const std::vector<const Road *> &roads,
                                                           double current_x, double current_y,
                                                           double target_x, double speed_x);
        std::pair<double, bool> CalculateVerticalMovement(const std::vector<const Road *> &roads,
                                                         double current_x, double current_y,
                                                         double target_y, double speed_y);
        std::pair<double, double> FindValidPosition(const std::vector<Road> &roads,
                                                   double current_x, double current_y,
                                                   double target_x, double target_y);
        double ClampHorizontalMovement(const model::Road &road, double current_x, double target_x, double speed_x);
        double ClampVerticalMovement(const model::Road &road, double current_y, double target_y, double speed_y);
    };

} // namespace model