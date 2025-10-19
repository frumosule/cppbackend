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
            
            // Вычисляем целевую позицию
            double target_x = current_x + speed_x * delta_time;
            double target_y = current_y + speed_y * delta_time;
            
            // Находим ВСЕ доступные дороги из текущей позиции
            auto connected_roads = FindConnectedRoads(roads, current_x, current_y);
            
            if (connected_roads.empty()) {
                // Игрок сошел с дороги - останавливаем
                player->SetSpeed(0.0, 0.0);
                continue;
            }
            
            // ОГРАНИЧИВАЕМ ДВИЖЕНИЕ
            bool boundary_reached = false;
            
            if (speed_x != 0.0) {
                auto [new_x, boundary] = CalculateMovementWithConnections(
                    connected_roads, current_x, current_y, target_x, target_y, speed_x, speed_y, true);
                target_x = new_x;
                if (boundary) boundary_reached = true;
            }
            
            if (speed_y != 0.0 && !boundary_reached) {
                auto [new_y, boundary] = CalculateMovementWithConnections(
                    connected_roads, current_x, current_y, target_x, target_y, speed_x, speed_y, false);
                target_y = new_y;
                if (boundary) boundary_reached = true;
            }
            
            // Если достигли границы, останавливаемся
            if (boundary_reached) {
                player->SetSpeed(0.0, 0.0);
            }
            
            // Устанавливаем новую позицию (ограниченную)
            player->SetPosition(target_x, target_y);
        }
    }
}

std::vector<const Road*> GameSessionManager::FindConnectedRoads(
    const std::vector<Road>& roads, double x, double y) {
    
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    std::vector<const Road*> connected_roads;
    
    // Находим дороги, на которых находится игрок
    std::vector<const Road*> current_roads;
    for (const auto& road : roads) {
        if (IsPositionOnRoad(road, x, y)) {
            current_roads.push_back(&road);
        }
    }
    
    // Если игрок не на дороге - возвращаем пустой список
    if (current_roads.empty()) {
        return connected_roads;
    }
    
    // Добавляем все соединенные дороги
    for (const auto& road : current_roads) {
        connected_roads.push_back(road);
        
        // Ищем соединенные дороги
        for (const auto& other_road : roads) {
            if (&other_road == road) continue;
            
            if (AreRoadsConnected(*road, other_road)) {
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
    const double CONNECTION_THRESHOLD = 0.4; // Половина ширины дороги
    
    auto endpoints1 = GetRoadEndpoints(road1);
    auto endpoints2 = GetRoadEndpoints(road2);
    
    // Проверяем все комбинации конечных точек
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

std::pair<double, bool> GameSessionManager::CalculateMovementWithConnections(
    const std::vector<const Road*>& connected_roads, double current_x, double current_y,
    double target_x, double target_y, double speed_x, double speed_y, bool is_horizontal) {
    
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    bool boundary_reached = false;
    double new_pos = is_horizontal ? target_x : target_y;
    
    if (is_horizontal) {
        // Горизонтальное движение
        double min_allowed_x = std::numeric_limits<double>::lowest();
        double max_allowed_x = std::numeric_limits<double>::max();
        
        bool found_valid_road = false;
        
        for (const auto& road : connected_roads) {
            if (road->IsHorizontal()) {
                const auto& start = road->GetStart();
                const auto& end = road->GetEnd();
                int min_x = std::min(start.x, end.x);
                int max_x = std::max(start.x, end.x);
                
                // Проверяем, что игрок может двигаться по Y на этой дороге
                if (current_y >= start.y - HALF_ROAD_WIDTH && current_y <= start.y + HALF_ROAD_WIDTH) {
                    found_valid_road = true;
                    min_allowed_x = std::max(min_allowed_x, static_cast<double>(min_x) - HALF_ROAD_WIDTH);
                    max_allowed_x = std::min(max_allowed_x, static_cast<double>(max_x) + HALF_ROAD_WIDTH);
                }
            }
        }
        
        if (found_valid_road) {
            // Ограничиваем движение
            if (speed_x > 0) {
                if (target_x > max_allowed_x) {
                    new_pos = max_allowed_x;
                    boundary_reached = true;
                }
            } else if (speed_x < 0) {
                if (target_x < min_allowed_x) {
                    new_pos = min_allowed_x;
                    boundary_reached = true;
                }
            }
        } else {
            // Не нашли подходящей дороги - останавливаемся
            boundary_reached = true;
            new_pos = current_x;
        }
        
    } else {
        // Вертикальное движение
        double min_allowed_y = std::numeric_limits<double>::lowest();
        double max_allowed_y = std::numeric_limits<double>::max();
        
        bool found_valid_road = false;
        
        for (const auto& road : connected_roads) {
            if (!road->IsHorizontal()) {
                const auto& start = road->GetStart();
                const auto& end = road->GetEnd();
                int min_y = std::min(start.y, end.y);
                int max_y = std::max(start.y, end.y);
                
                // Проверяем, что игрок может двигаться по X на этой дороге
                if (current_x >= start.x - HALF_ROAD_WIDTH && current_x <= start.x + HALF_ROAD_WIDTH) {
                    found_valid_road = true;
                    min_allowed_y = std::max(min_allowed_y, static_cast<double>(min_y) - HALF_ROAD_WIDTH);
                    max_allowed_y = std::min(max_allowed_y, static_cast<double>(max_y) + HALF_ROAD_WIDTH);
                }
            }
        }
        
        if (found_valid_road) {
            if (speed_y > 0) {
                if (target_y > max_allowed_y) {
                    new_pos = max_allowed_y;
                    boundary_reached = true;
                }
            } else if (speed_y < 0) {
                if (target_y < min_allowed_y) {
                    new_pos = min_allowed_y;
                    boundary_reached = true;
                }
            }
        } else {
            boundary_reached = true;
            new_pos = current_y;
        }
    }
    
    return {new_pos, boundary_reached};
}

bool GameSessionManager::IsPositionOnRoad(const Road& road, double x, double y) {
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    const auto& start = road.GetStart();
    
    if (road.IsHorizontal()) {
        const auto& end = road.GetEnd();
        int min_x = std::min(start.x, end.x);
        int max_x = std::max(start.x, end.x);

        // Проверяем что позиция находится в пределах дороги
        bool within_x = (x >= min_x - HALF_ROAD_WIDTH) && (x <= max_x + HALF_ROAD_WIDTH);
        bool within_y = (y >= start.y - HALF_ROAD_WIDTH) && (y <= start.y + HALF_ROAD_WIDTH);
        
        return within_x && within_y;
    } else {
        const auto& end = road.GetEnd();
        int min_y = std::min(start.y, end.y);
        int max_y = std::max(start.y, end.y);

        bool within_x = (x >= start.x - HALF_ROAD_WIDTH) && (x <= start.x + HALF_ROAD_WIDTH);
        bool within_y = (y >= min_y - HALF_ROAD_WIDTH) && (y <= max_y + HALF_ROAD_WIDTH);
        
        return within_x && within_y;
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