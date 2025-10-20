#include "game_session_manager.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex> 
#include <algorithm>
#include <limits>
#include <cmath>

namespace model {

std::shared_ptr<GameSession> GameSessionManager::FindSession(const Map::Id& map_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return FindSessionUnsafe(map_id);
}

std::string GameSessionManager::GenerateAuthToken() {
    uint64_t random_value = distribution_(generator_);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << random_value;
    
    uint64_t random_value2 = distribution_(generator_);
    ss << std::hex << std::setfill('0') << std::setw(16) << random_value2;
    
    return ss.str();
}

std::shared_ptr<Player> GameSessionManager::AddPlayer(const Map::Id& map_id, const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto session = FindSessionUnsafe(map_id);
    if (!session) {
        session = CreateSessionUnsafe(map_id);
    }

    auto [pos_x, pos_y] = GetRandomPositionOnRoad(map_id);
    
    std::string auth_token = GenerateAuthToken();
    auto player = std::make_shared<Player>(next_player_id_++, player_name, auth_token, map_id, pos_x, pos_y);
    
    session->AddPlayer(player);
    players_by_token_[auth_token] = player;
    players_by_id_[player->GetId()] = player;
    
    return player;
}

std::shared_ptr<Player> GameSessionManager::FindPlayerByToken(const std::string& auth_token) {
    std::lock_guard<std::mutex> lock(mutex_);
    return FindPlayerByTokenUnsafe(auth_token);
}

std::shared_ptr<Player> GameSessionManager::FindPlayerById(Player::Id player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = players_by_id_.find(player_id);
    return it != players_by_id_.end() ? it->second : nullptr;
}

bool GameSessionManager::UpdatePlayerAction(const std::string& auth_token, const std::string& move, double dog_speed) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player = FindPlayerByTokenUnsafe(auth_token);
    if (!player) {
        return false;
    }
    
    if (move == "L") {
        player->SetSpeed(-dog_speed, 0.0);
    } else if (move == "R") {
        player->SetSpeed(dog_speed, 0.0);
    } else if (move == "U") {
        player->SetSpeed(0.0, -dog_speed);
    } else if (move == "D") {
        player->SetSpeed(0.0, dog_speed);
    } else if (move == "") {
        player->SetSpeed(0.0, 0.0);
    }
    
    return true;
}

std::unordered_map<Player::Id, std::shared_ptr<Player>> GameSessionManager::GetPlayersByToken(const std::string& auth_token) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto current_player = FindPlayerByTokenUnsafe(auth_token);
    if (!current_player) {
        return {};
    }
    
    auto session = FindSessionUnsafe(current_player->GetMapId());
    if (!session) {
        return {};
    }
    
    return session->GetPlayers();
}

std::pair<double, double> GameSessionManager::GetRandomPositionOnRoad(const Map::Id& map_id) {
    auto map = game_.FindMap(map_id);
    if (!map) {
        throw std::runtime_error("Map not found for getting random position");
    }
    
    const auto& roads = map->GetRoads();
    if (roads.empty()) {
        throw std::runtime_error("No roads on the map");
    }

    // ВРЕМЕННО: всегда возвращаем начало первой дороги для тестирования
    const auto& first_road = roads[0];
    const auto& start = first_road.GetStart();

    return {static_cast<double>(start.x), static_cast<double>(start.y)};
}

void GameSessionManager::UpdateGameState(int time_delta_ms) {
    std::lock_guard<std::mutex> lock(mutex_);    
    
    double delta_time = time_delta_ms / 1000.0;
    
    for (auto& [map_id, session] : sessions_) {
        auto map = game_.FindMap(map_id);
        if (!map) continue;
        
        const auto& roads = map->GetRoads();
        
        for (auto& [player_id, player] : session->GetPlayers()) {
            if (player->GetSpeedX() == 0.0 && player->GetSpeedY() == 0.0) {
                continue;
            }
            
            double current_x = player->GetPosX();
            double current_y = player->GetPosY();
            double speed_x = player->GetSpeedX();
            double speed_y = player->GetSpeedY();
            
            double target_x = current_x + speed_x * delta_time;
            double target_y = current_y + speed_y * delta_time;
            
            // ПРОСТАЯ ЛОГИКА: проверяем ВСЕ дороги на карте
            bool can_move_to_target = false;
            for (const auto& road : roads) {
                if (IsPositionOnRoad(road, target_x, target_y)) {
                    can_move_to_target = true;
                    break;
                }
            }
            
            if (can_move_to_target) {
                player->SetPosition(target_x, target_y);
            } else {
                // Если не можем в target, ищем максимально возможное движение
                bool found_valid = false;
                double best_x = current_x;
                double best_y = current_y;
                
                // БИНАРНЫЙ ПОИСК для точности
                double min_t = 0.0;
                double max_t = 1.0;
                
                for (int i = 0; i < 20; i++) {
                    double mid_t = (min_t + max_t) / 2.0;
                    double test_x = current_x + (target_x - current_x) * mid_t;
                    double test_y = current_y + (target_y - current_y) * mid_t;
                    
                    bool test_valid = false;
                    for (const auto& road : roads) {
                        if (IsPositionOnRoad(road, test_x, test_y)) {
                            test_valid = true;
                            best_x = test_x;
                            best_y = test_y;
                            break;
                        }
                    }
                    
                    if (test_valid) {
                        min_t = mid_t; // Можем двигаться дальше
                        found_valid = true;
                    } else {
                        max_t = mid_t; // Не можем - уменьшаем шаг
                    }
                }
                
                if (found_valid) {
                    player->SetPosition(best_x, best_y);
                }
                
                // Останавливаемся только если не смогли сдвинуться значительно
                double distance_moved = std::hypot(best_x - current_x, best_y - current_y);
                if (distance_moved < 1e-6) {
                    player->SetSpeed(0.0, 0.0);
                }
            }
        }
    }
}

std::vector<const Road*> GameSessionManager::FindConnectedRoads(
    const std::vector<Road>& roads, double x, double y) {
    
    std::vector<const Road*> connected_roads;
    
    // 1. Находим дороги под игроком
    for (const auto& road : roads) {
        if (IsPositionOnRoad(road, x, y)) {
            connected_roads.push_back(&road);
        }
    }
    
    // 2. Находим дороги, соединенные с найденными дорогами
    for (const auto& current_road : connected_roads) {
        for (const auto& other_road : roads) {
            if (&other_road == current_road) continue;
            
            if (AreRoadsConnected(*current_road, other_road)) {
                // Проверяем, что дорога еще не добавлена
                if (std::find(connected_roads.begin(), connected_roads.end(), &other_road) == connected_roads.end()) {
                    connected_roads.push_back(&other_road);
                }
            }
        }
    }
    
    return connected_roads;
}

bool GameSessionManager::AreRoadsConnected(const Road& road1, const Road& road2) {
    const double CONNECTION_THRESHOLD = 0.4;
    
    auto endpoints1 = GetRoadEndpoints(road1);
    auto endpoints2 = GetRoadEndpoints(road2);
    
    for (const auto& point1 : endpoints1) {
        for (const auto& point2 : endpoints2) {
            double distance = std::hypot(point1.first - point2.first, point1.second - point2.second);
            if (distance <= CONNECTION_THRESHOLD) {
                return true;
            }
        }
    }
    
    return false;
}

std::vector<std::pair<double, double>> GameSessionManager::GetRoadEndpoints(const Road& road) {
    std::vector<std::pair<double, double>> endpoints;
    
    const auto& start = road.GetStart();
    if (road.IsHorizontal()) {
        const auto& end = road.GetEnd();
        endpoints.push_back({static_cast<double>(start.x), static_cast<double>(start.y)});
        endpoints.push_back({static_cast<double>(end.x), static_cast<double>(start.y)});
    } else {
        const auto& end = road.GetEnd();
        endpoints.push_back({static_cast<double>(start.x), static_cast<double>(start.y)});
        endpoints.push_back({static_cast<double>(start.x), static_cast<double>(end.y)});
    }
    
    return endpoints;
}

bool GameSessionManager::IsPositionOnRoad(const Road& road, double x, double y) {
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    const auto& start = road.GetStart();
    
    if (road.IsHorizontal()) {
        const auto& end = road.GetEnd();
        int min_x = std::min(start.x, end.x);
        int max_x = std::max(start.x, end.x);

        bool within_x = (x >= min_x - HALF_ROAD_WIDTH) && (x <= max_x + HALF_ROAD_WIDTH);
        bool within_y = (y >= start.y - HALF_ROAD_WIDTH) && (y <= start.y + HALF_ROAD_WIDTH);
        
        bool result = within_x && within_y;
        
        return result;
    } else {
        const auto& end = road.GetEnd();
        int min_y = std::min(start.y, end.y);
        int max_y = std::max(start.y, end.y);

        bool within_x = (x >= start.x - HALF_ROAD_WIDTH) && (x <= start.x + HALF_ROAD_WIDTH);
        bool within_y = (y >= min_y - HALF_ROAD_WIDTH) && (y <= max_y + HALF_ROAD_WIDTH);
        
        bool result = within_x && within_y;
        
        return result;
    }
}

// Старые методы для обратной совместимости
const model::Road* GameSessionManager::FindRoadForPosition(const std::vector<model::Road>& roads, double x, double y) {
    for (const auto& road : roads) {
        if (IsPositionOnRoad(road, x, y)) {
            return &road;
        }
    }
    return nullptr;
}

double GameSessionManager::ClampHorizontalMovement(const model::Road& road, double current_x, double target_x, double speed_x) {
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    const auto& start = road.GetStart();
    const auto& end = road.GetEnd();
    
    int min_x = std::min(start.x, end.x);
    int max_x = std::max(start.x, end.x);
    
    double road_min_x = min_x - HALF_ROAD_WIDTH;
    double road_max_x = max_x + HALF_ROAD_WIDTH;
    
    if (speed_x > 0) {
        return std::min(target_x, road_max_x);
    } else {
        return std::max(target_x, road_min_x);
    }
}

double GameSessionManager::ClampVerticalMovement(const model::Road& road, double current_y, double target_y, double speed_y) {
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    const auto& start = road.GetStart();
    const auto& end = road.GetEnd();
    
    int min_y = std::min(start.y, end.y);
    int max_y = std::max(start.y, end.y);
    
    double road_min_y = min_y - HALF_ROAD_WIDTH;
    double road_max_y = max_y + HALF_ROAD_WIDTH;
    
    if (speed_y > 0) {
        return std::min(target_y, road_max_y);
    } else {
        return std::max(target_y, road_min_y);
    }
}

std::shared_ptr<GameSession> GameSessionManager::FindSessionUnsafe(const Map::Id& map_id) {
    auto it = sessions_.find(map_id);
    return it != sessions_.end() ? it->second : nullptr;
}

std::shared_ptr<GameSession> GameSessionManager::CreateSessionUnsafe(const Map::Id& map_id) {
    auto session = std::make_shared<GameSession>(map_id);
    sessions_[map_id] = session;
    return session;
}

std::shared_ptr<Player> GameSessionManager::FindPlayerByTokenUnsafe(const std::string& auth_token) {
    auto it = players_by_token_.find(auth_token);
    return it != players_by_token_.end() ? it->second : nullptr;
}

} // namespace model