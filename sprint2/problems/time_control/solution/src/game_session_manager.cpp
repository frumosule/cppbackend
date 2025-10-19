#include "game_session_manager.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex> 
#include <algorithm>
#include <limits>

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

    // ВРЕМЕННО: используем начало первой дороги вместо случайной позиции
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
            
            // Находим ВСЕ дороги, на которых находится игрок
            std::vector<const Road*> possible_roads;
            for (const auto& road : roads) {
                if (IsPositionOnRoad(road, current_x, current_y)) {
                    possible_roads.push_back(&road);
                }
            }
            
            if (possible_roads.empty()) {
                player->SetSpeed(0.0, 0.0);
                continue;
            }
            
            // Ограничиваем движение с учетом ВСЕХ возможных дорог
            bool reached_boundary = false;
            
            if (speed_x != 0.0) {
                auto [new_x, boundary] = CalculateHorizontalMovement(possible_roads, current_x, current_y, target_x, speed_x);
                if (boundary) reached_boundary = true;
                target_x = new_x;
            } else if (speed_y != 0.0) {
                auto [new_y, boundary] = CalculateVerticalMovement(possible_roads, current_x, current_y, target_y, speed_y);
                if (boundary) reached_boundary = true;
                target_y = new_y;
            }
            
            // Проверяем, остался ли игрок на дороге после перемещения
            bool still_on_road = false;
            for (const auto& road : roads) {
                if (IsPositionOnRoad(road, target_x, target_y)) {
                    still_on_road = true;
                    break;
                }
            }
            
            if (!still_on_road) {
                // Если ушел с дороги, ищем ближайшую допустимую позицию
                auto [valid_x, valid_y] = FindValidPosition(roads, current_x, current_y, target_x, target_y);
                target_x = valid_x;
                target_y = valid_y;
                reached_boundary = true;
            }
            
            if (reached_boundary) {
                player->SetSpeed(0.0, 0.0);
            }
            
            player->SetPosition(target_x, target_y);
        }
    }
}

bool GameSessionManager::IsPositionOnRoad(const Road& road, double x, double y) {
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    const auto& start = road.GetStart();
    
    if (road.IsHorizontal()) {
        const auto& end = road.GetEnd();
        int min_x = std::min(start.x, end.x);
        int max_x = std::max(start.x, end.x);

        return (x >= min_x - HALF_ROAD_WIDTH && x <= max_x + HALF_ROAD_WIDTH &&
                y >= start.y - HALF_ROAD_WIDTH && y <= start.y + HALF_ROAD_WIDTH);
    } else {
        const auto& end = road.GetEnd();
        int min_y = std::min(start.y, end.y);
        int max_y = std::max(start.y, end.y);

        return (x >= start.x - HALF_ROAD_WIDTH && x <= start.x + HALF_ROAD_WIDTH &&
                y >= min_y - HALF_ROAD_WIDTH && y <= max_y + HALF_ROAD_WIDTH);
    }
}

std::pair<double, bool> GameSessionManager::CalculateHorizontalMovement(
    const std::vector<const Road*>& roads, double current_x, double current_y, 
    double target_x, double speed_x) {
    
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    bool boundary_reached = false;
    double new_x = target_x;
    
    // Находим минимальное и максимальное возможное X среди всех дорог
    double min_allowed_x = std::numeric_limits<double>::lowest();
    double max_allowed_x = std::numeric_limits<double>::max();
    
    for (const auto& road : roads) {
        if (road->IsHorizontal()) {
            const auto& start = road->GetStart();
            const auto& end = road->GetEnd();
            int min_x = std::min(start.x, end.x);
            int max_x = std::max(start.x, end.x);
            
            // Проверяем, что игрок может двигаться по этой дороге по Y
            if (current_y >= start.y - HALF_ROAD_WIDTH && current_y <= start.y + HALF_ROAD_WIDTH) {
                min_allowed_x = std::max(min_allowed_x, static_cast<double>(min_x) - HALF_ROAD_WIDTH);
                max_allowed_x = std::min(max_allowed_x, static_cast<double>(max_x) + HALF_ROAD_WIDTH);
            }
        }
    }
    
    // Если не нашли подходящих дорог - останавливаемся
    if (min_allowed_x == std::numeric_limits<double>::lowest() || 
        max_allowed_x == std::numeric_limits<double>::max()) {
        return {current_x, true};
    }
    
    // Ограничиваем движение
    if (speed_x > 0) { // Движение вправо
        if (target_x > max_allowed_x) {
            new_x = max_allowed_x;
            boundary_reached = true;
        }
    } else { // Движение влево
        if (target_x < min_allowed_x) {
            new_x = min_allowed_x;
            boundary_reached = true;
        }
    }
    
    return {new_x, boundary_reached};
}

std::pair<double, bool> GameSessionManager::CalculateVerticalMovement(
    const std::vector<const Road*>& roads, double current_x, double current_y, 
    double target_y, double speed_y) {
    
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    bool boundary_reached = false;
    double new_y = target_y;
    
    // Находим минимальное и максимальное возможное Y среди всех дорог
    double min_allowed_y = std::numeric_limits<double>::lowest();
    double max_allowed_y = std::numeric_limits<double>::max();
    
    for (const auto& road : roads) {
        if (!road->IsHorizontal()) {
            const auto& start = road->GetStart();
            const auto& end = road->GetEnd();
            int min_y = std::min(start.y, end.y);
            int max_y = std::max(start.y, end.y);
            
            // Проверяем, что игрок может двигаться по этой дороге по X
            if (current_x >= start.x - HALF_ROAD_WIDTH && current_x <= start.x + HALF_ROAD_WIDTH) {
                min_allowed_y = std::max(min_allowed_y, static_cast<double>(min_y) - HALF_ROAD_WIDTH);
                max_allowed_y = std::min(max_allowed_y, static_cast<double>(max_y) + HALF_ROAD_WIDTH);
            }
        }
    }
    
    // Если не нашли подходящих дорог - останавливаемся
    if (min_allowed_y == std::numeric_limits<double>::lowest() || 
        max_allowed_y == std::numeric_limits<double>::max()) {
        return {current_y, true};
    }
    
    // Ограничиваем движение
    if (speed_y > 0) { // Движение вниз
        if (target_y > max_allowed_y) {
            new_y = max_allowed_y;
            boundary_reached = true;
        }
    } else { // Движение вверх
        if (target_y < min_allowed_y) {
            new_y = min_allowed_y;
            boundary_reached = true;
        }
    }
    
    return {new_y, boundary_reached};
}

std::pair<double, double> GameSessionManager::FindValidPosition(
    const std::vector<Road>& roads, double current_x, double current_y,
    double target_x, double target_y) {
    
    // Ищем позицию на линии между текущей и целевой, которая находится на дороге
    double dx = target_x - current_x;
    double dy = target_y - current_y;
    double distance = std::hypot(dx, dy);
    
    if (distance == 0) {
        return {current_x, current_y};
    }
    
    // Делим путь на маленькие шаги и ищем максимально допустимое перемещение
    const int STEPS = 100;
    double step = distance / STEPS;
    double best_x = current_x;
    double best_y = current_y;
    
    for (int i = 1; i <= STEPS; ++i) {
        double test_x = current_x + (dx * i * step) / distance;
        double test_y = current_y + (dy * i * step) / distance;
        
        bool on_road = false;
        for (const auto& road : roads) {
            if (IsPositionOnRoad(road, test_x, test_y)) {
                on_road = true;
                break;
            }
        }
        
        if (on_road) {
            best_x = test_x;
            best_y = test_y;
        } else {
            break;
        }
    }
    
    return {best_x, best_y};
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