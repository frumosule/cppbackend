#include "game_session_manager.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex> 

namespace model {

std::shared_ptr<GameSession> GameSessionManager::FindSession(const Map::Id& map_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return FindSessionUnsafe(map_id);
}

std::string GameSessionManager::GenerateAuthToken() {
    uint64_t random_value = distribution_(generator_);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << random_value;
    
    // ╨У╨╡╨╜╨╡╤А╨╕╤А╤Г╨╡╨╝ ╨▓╤В╨╛╤А╨╛╨╣ ╤Б╨╗╤Г╤З╨░╨╣╨╜╤Л╨╣ ╨╜╨╛╨╝╨╡╤А ╨┤╨╗╤П ╨┐╨╛╨╗╨╜╨╛╨│╨╛ 32-╤Б╨╕╨╝╨▓╨╛╨╗╤М╨╜╨╛╨│╨╛ ╤В╨╛╨║╨╡╨╜╨░
    uint64_t random_value2 = distribution_(generator_);
    ss << std::hex << std::setfill('0') << std::setw(16) << random_value2;
    
    return ss.str();
}

std::shared_ptr<Player> GameSessionManager::AddPlayer(const Map::Id& map_id, const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ╨Ш╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╨╝ unsafe ╨╝╨╡╤В╨╛╨┤╤Л ╨▓╨╜╤Г╤В╤А╨╕
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
    std::lock_guard<std::mutex> lock(mutex_);  // ╨Ч╨░╤Й╨╕╤Й╨░╨╡╨╝ ╨┤╨╛╤Б╤В╤Г╨┐
    auto it = players_by_id_.find(player_id);
    return it != players_by_id_.end() ? it->second : nullptr;
}

bool GameSessionManager::UpdatePlayerAction(const std::string& auth_token, const std::string& move, double dog_speed) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player = FindPlayerByTokenUnsafe(auth_token);
    if (!player) {
        return false;
    }
    
    // ╨г╤Б╤В╨░╨╜╨░╨▓╨╗╨╕╨▓╨░╨╡╨╝ ╤Б╨║╨╛╤А╨╛╤Б╤В╤М
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
    
    // ╨Э╨░╤Е╨╛╨┤╨╕╨╝ ╨╕╨│╤А╨╛╨║╨░ ╨┐╨╛ ╤В╨╛╨║╨╡╨╜╤Г
    auto current_player = FindPlayerByTokenUnsafe(auth_token);
    if (!current_player) {
        return {};
    }
    
    // ╨Э╨░╤Е╨╛╨┤╨╕╨╝ ╤Б╨╡╤Б╤Б╨╕╤О ╤Н╤В╨╛╨│╨╛ ╨╕╨│╤А╨╛╨║╨░
    auto session = FindSessionUnsafe(current_player->GetMapId());
    if (!session) {
        return {};
    }
    
    // ╨Т╨╛╨╖╨▓╤А╨░╤Й╨░╨╡╨╝ ╨▓╤Б╨╡╤Е ╨╕╨│╤А╨╛╨║╨╛╨▓ ╨▓ ╤Н╤В╨╛╨╣ ╤Б╨╡╤Б╤Б╨╕╨╕
    return session->GetPlayers();
}

std::pair<double, double> GameSessionManager::GetRandomPositionOnRoad(const Map::Id& map_id) {
    // ╨в╨╡╨┐╨╡╤А╤М game_ ╨┤╨╛╤Б╤В╤Г╨┐╨╡╨╜ ╤З╨╡╤А╨╡╨╖ ╤Б╤Б╤Л╨╗╨║╤Г ╨▓ ╨║╨╗╨░╤Б╤Б╨╡
    auto map = game_.FindMap(map_id);
    if (!map) {
        throw std::runtime_error("Map not found for getting random position");
    }
    
    const auto& roads = map->GetRoads();
    if (roads.empty()) {
        throw std::runtime_error("No roads on the map");
    }

    // ╨Т╨а╨Х╨Ь╨Х╨Э╨Э╨Ю: ╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╨╝ ╨╜╨░╤З╨░╨╗╨╛ ╨┐╨╡╤А╨▓╨╛╨╣ ╨┤╨╛╤А╨╛╨│╨╕ ╨▓╨╝╨╡╤Б╤В╨╛ ╤Б╨╗╤Г╤З╨░╨╣╨╜╨╛╨╣ ╨┐╨╛╨╖╨╕╤Ж╨╕╨╕
    const auto& first_road = roads[0];
    const auto& start = first_road.GetStart();
    
    return {static_cast<double>(start.x), static_cast<double>(start.y)};
    
    // ╨Т╤Л╨▒╨╕╤А╨░╨╡╨╝ ╤Б╨╗╤Г╤З╨░╨╣╨╜╤Г╤О ╨┤╨╛╤А╨╛╨│╤Г
    //std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
    //const auto& road = roads[road_dist(generator_)];
    
    // ╨Т╤Л╨▒╨╕╤А╨░╨╡╨╝ ╤Б╨╗╤Г╤З╨░╨╣╨╜╤Г╤О ╨┐╨╛╨╖╨╕╤Ж╨╕╤О ╨╜╨░ ╨┤╨╛╤А╨╛╨│╨╡
    //const auto& start = road.GetStart();
    
    //if (road.IsHorizontal()) {
        //const auto& end = road.GetEnd();
        //std::uniform_real_distribution<double> pos_dist(static_cast<double>(start.x), static_cast<double>(end.x));
        //double x = pos_dist(generator_);
        //double y = static_cast<double>(start.y);
        //return std::make_pair(x, y);
    //} else {
        //const auto& end = road.GetEnd();
        //std::uniform_real_distribution<double> pos_dist(static_cast<double>(start.y), static_cast<double>(end.y));
        //double y = pos_dist(generator_);
        //double x = static_cast<double>(start.x);
        //return std::make_pair(x, y);
    //}
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
            
            // ПРОСТОЕ РЕШЕНИЕ: проверяем ВСЕ дороги карты для целевой позиции
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
                // Ищем максимальное движение по ВСЕМ дорогам
                double best_x = current_x;
                double best_y = current_y;
                bool found_better = false;
                
                for (int i = 1; i <= 100; i++) {
                    double t = i / 100.0;
                    double test_x = current_x + (target_x - current_x) * t;
                    double test_y = current_y + (target_y - current_y) * t;
                    
                    bool valid = false;
                    for (const auto& road : roads) {  // Проверяем ВСЕ дороги
                        if (IsPositionOnRoad(road, test_x, test_y)) {
                            valid = true;
                            best_x = test_x;
                            best_y = test_y;
                            found_better = true;
                            break;
                        }
                    }
                    
                    if (!valid) break;
                }
                
                if (found_better) {
                    player->SetPosition(best_x, best_y);
                    player->SetSpeed(0.0, 0.0);
                } else {
                    player->SetSpeed(0.0, 0.0);
                }
            }
        }
    }
}

const model::Road* GameSessionManager::FindRoadForPosition(const std::vector<model::Road>& roads, double x, double y) {
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    for (const auto& road : roads) {
        const auto& start = road.GetStart();
        
        if (road.IsHorizontal()) {
            const auto& end = road.GetEnd();
            int min_x = std::min(start.x, end.x);
            int max_x = std::max(start.x, end.x);

             bool in_road = (x >= min_x - HALF_ROAD_WIDTH && x <= max_x + HALF_ROAD_WIDTH &&
                           y >= start.y - HALF_ROAD_WIDTH && y <= start.y + HALF_ROAD_WIDTH);
            
            // ╨Я╤А╨╛╨▓╨╡╤А╤П╨╡╨╝, ╨╜╨░╤Е╨╛╨┤╨╕╤В╤Б╤П ╨╗╨╕ ╨┐╨╛╨╖╨╕╤Ж╨╕╤П ╨▓ ╨┐╤А╨╡╨┤╨╡╨╗╨░╤Е ╨┤╨╛╤А╨╛╨│╨╕
            if (x >= min_x - HALF_ROAD_WIDTH && x <= max_x + HALF_ROAD_WIDTH &&
                y >= start.y - HALF_ROAD_WIDTH && y <= start.y + HALF_ROAD_WIDTH) {
                return &road;
            }
        } else {
            const auto& end = road.GetEnd();
            int min_y = std::min(start.y, end.y);
            int max_y = std::max(start.y, end.y);

            bool in_road = (x >= start.x - HALF_ROAD_WIDTH && x <= start.x + HALF_ROAD_WIDTH &&
                           y >= min_y - HALF_ROAD_WIDTH && y <= max_y + HALF_ROAD_WIDTH);
            
            // ╨Я╤А╨╛╨▓╨╡╤А╤П╨╡╨╝, ╨╜╨░╤Е╨╛╨┤╨╕╤В╤Б╤П ╨╗╨╕ ╨┐╨╛╨╖╨╕╤Ж╨╕╤П ╨▓ ╨┐╤А╨╡╨┤╨╡╨╗╨░╤Е ╨┤╨╛╤А╨╛╨│╨╕
            if (x >= start.x - HALF_ROAD_WIDTH && x <= start.x + HALF_ROAD_WIDTH &&
                y >= min_y - HALF_ROAD_WIDTH && y <= max_y + HALF_ROAD_WIDTH) {
                return &road;
            }
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
    
    if (speed_x > 0) { // ╨Ф╨▓╨╕╨╢╨╡╨╜╨╕╨╡ ╨▓╨┐╤А╨░╨▓╨╛
        return std::min(target_x, road_max_x);
    } else { // ╨Ф╨▓╨╕╨╢╨╡╨╜╨╕╨╡ ╨▓╨╗╨╡╨▓╨╛
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
    
    if (speed_y > 0) { // ╨Ф╨▓╨╕╨╢╨╡╨╜╨╕╨╡ ╨▓╨╜╨╕╨╖
        return std::min(target_y, road_max_y);
    } else { // ╨Ф╨▓╨╕╨╢╨╡╨╜╨╕╨╡ ╨▓╨▓╨╡╤А╤Е
        return std::max(target_y, road_min_y);
    }
}

std::vector<const Road*> GameSessionManager::FindAllRoadsForPosition(const std::vector<Road>& roads, double x, double y) {
    std::vector<const Road*> result;
    const double ROAD_WIDTH = 0.8;
    const double HALF_ROAD_WIDTH = ROAD_WIDTH / 2.0;
    
    std::cout << "DEBUG: Finding roads for position (" << x << ", " << y << ")" << std::endl;
    
    for (const auto& road : roads) {
        const auto& start = road.GetStart();
        
        if (road.IsHorizontal()) {
            const auto& end = road.GetEnd();
            int min_x = std::min(start.x, end.x);
            int max_x = std::max(start.x, end.x);

            bool in_road = (x >= min_x - HALF_ROAD_WIDTH && x <= max_x + HALF_ROAD_WIDTH &&
                           y >= start.y - HALF_ROAD_WIDTH && y <= start.y + HALF_ROAD_WIDTH);
            
            if (in_road) {
                std::cout << "DEBUG: Found HORIZONTAL road from (" << start.x << "," << start.y 
                         << ") to (" << end.x << "," << start.y << ")" << std::endl;
                result.push_back(&road);
            }
        } else {
            const auto& end = road.GetEnd();
            int min_y = std::min(start.y, end.y);
            int max_y = std::max(start.y, end.y);

            bool in_road = (x >= start.x - HALF_ROAD_WIDTH && x <= start.x + HALF_ROAD_WIDTH &&
                           y >= min_y - HALF_ROAD_WIDTH && y <= max_y + HALF_ROAD_WIDTH);
            
            if (in_road) {
                std::cout << "DEBUG: Found VERTICAL road from (" << start.x << "," << start.y 
                         << ") to (" << start.x << "," << end.y << ")" << std::endl;
                result.push_back(&road);
            }
        }
    }
    
    std::cout << "DEBUG: Total roads found: " << result.size() << std::endl;
    return result;
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
        
        return within_x && within_y;
    } else {
        const auto& end = road.GetEnd();
        int min_y = std::min(start.y, end.y);
        int max_y = std::max(start.y, end.y);

        // ИСПРАВЛЕНИЕ: учитываем полную ширину дороги до конца
        bool within_x = (x >= start.x - HALF_ROAD_WIDTH) && (x <= start.x + HALF_ROAD_WIDTH);
        bool within_y = (y >= min_y - HALF_ROAD_WIDTH) && (y <= max_y + HALF_ROAD_WIDTH);
        
        return within_x && within_y;
    }
}

} // namespace model

