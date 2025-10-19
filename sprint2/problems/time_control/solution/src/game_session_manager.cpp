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
    
    // Генерируем второй случайный номер для полного 32-символьного токена
    uint64_t random_value2 = distribution_(generator_);
    ss << std::hex << std::setfill('0') << std::setw(16) << random_value2;
    
    return ss.str();
}

std::shared_ptr<Player> GameSessionManager::AddPlayer(const Map::Id& map_id, const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Используем unsafe методы внутри
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
    std::lock_guard<std::mutex> lock(mutex_);  // Защищаем доступ
    auto it = players_by_id_.find(player_id);
    return it != players_by_id_.end() ? it->second : nullptr;
}

bool GameSessionManager::UpdatePlayerAction(const std::string& auth_token, const std::string& move, double dog_speed) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player = FindPlayerByTokenUnsafe(auth_token);
    if (!player) {
        return false;
    }
    
    // Устанавливаем скорость
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
    
    // Находим игрока по токену
    auto current_player = FindPlayerByTokenUnsafe(auth_token);
    if (!current_player) {
        return {};
    }
    
    // Находим сессию этого игрока
    auto session = FindSessionUnsafe(current_player->GetMapId());
    if (!session) {
        return {};
    }
    
    // Возвращаем всех игроков в этой сессии
    return session->GetPlayers();
}

std::pair<double, double> GameSessionManager::GetRandomPositionOnRoad(const Map::Id& map_id) {
    // Теперь game_ доступен через ссылку в классе
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
    
    // Выбираем случайную дорогу
    //std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
    //const auto& road = roads[road_dist(generator_)];
    
    // Выбираем случайную позицию на дороге
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
    
    double delta_time = time_delta_ms / 1000.0; // Конвертируем в секунды
    
    for (auto& [map_id, session] : sessions_) {
        auto map = game_.FindMap(map_id);
        if (!map) continue;
        
        const auto& roads = map->GetRoads();
        
        for (auto& [player_id, player] : session->GetPlayers()) {
            if (player->GetSpeedX() == 0.0 && player->GetSpeedY() == 0.0) {
                continue; // Игрок не движется
            }
            
            double current_x = player->GetPosX();
            double current_y = player->GetPosY();
            double speed_x = player->GetSpeedX();
            double speed_y = player->GetSpeedY();
            
            // Вычисляем целевую позицию
            double target_x = current_x + speed_x * delta_time;
            double target_y = current_y + speed_y * delta_time;
            
            // Находим дорогу, на которой находится игрок
            auto current_road = FindRoadForPosition(roads, current_x, current_y);
            if (!current_road) {
                // Игрок не на дороге - останавливаем
                player->SetSpeed(0.0, 0.0);
                continue;
            }
            
            // Проверяем движение по текущей дороге
            if (speed_x != 0.0) { // Горизонтальное движение
                target_x = ClampHorizontalMovement(*current_road, current_x, target_x, speed_x);
            } else if (speed_y != 0.0) { // Вертикальное движение
                target_y = ClampVerticalMovement(*current_road, current_y, target_y, speed_y);
            }
            
            // Проверяем, достигли ли мы границы дороги
            bool reached_boundary = false;
            if (speed_x > 0 && target_x <= current_x) reached_boundary = true;
            if (speed_x < 0 && target_x >= current_x) reached_boundary = true;
            if (speed_y > 0 && target_y <= current_y) reached_boundary = true;
            if (speed_y < 0 && target_y >= current_y) reached_boundary = true;
            
            if (reached_boundary) {
                // Достигли границы - останавливаемся
                player->SetSpeed(0.0, 0.0);
                // Оставляем позицию на границе
                target_x = current_x;
                target_y = current_y;
            }
            
            player->SetPosition(target_x, target_y);
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
            
            // Проверяем, находится ли позиция в пределах дороги
            if (x >= min_x - HALF_ROAD_WIDTH && x <= max_x + HALF_ROAD_WIDTH &&
                y >= start.y - HALF_ROAD_WIDTH && y <= start.y + HALF_ROAD_WIDTH) {
                return &road;
            }
        } else {
            const auto& end = road.GetEnd();
            int min_y = std::min(start.y, end.y);
            int max_y = std::max(start.y, end.y);
            
            // Проверяем, находится ли позиция в пределах дороги
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
    
    if (speed_x > 0) { // Движение вправо
        return std::min(target_x, road_max_x);
    } else { // Движение влево
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
    
    if (speed_y > 0) { // Движение вниз
        return std::min(target_y, road_max_y);
    } else { // Движение вверх
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

