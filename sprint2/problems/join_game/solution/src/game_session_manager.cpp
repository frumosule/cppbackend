#include "game_session_manager.h"
#include <sstream>
#include <iomanip>

namespace model {

std::shared_ptr<GameSession> GameSessionManager::FindSession(const Map::Id& map_id) {
    auto it = sessions_.find(map_id);
    return it != sessions_.end() ? it->second : nullptr;
}

std::shared_ptr<GameSession> GameSessionManager::CreateSession(const Map::Id& map_id) {
    auto session = std::make_shared<GameSession>(map_id);
    sessions_[map_id] = session;
    return session;
}

std::string GameSessionManager::GenerateAuthToken() {
    uint64_t random_value = distribution_(generator_);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << random_value;
    return ss.str();
}

std::shared_ptr<Player> GameSessionManager::AddPlayer(const Map::Id& map_id, const std::string& player_name) {
    // Создаем или находим сессию для карты
    auto session = FindSession(map_id);
    if (!session) {
        session = CreateSession(map_id);
    }
    
    // Генерируем токен
    std::string auth_token = GenerateAuthToken();
    
    // Создаем игрока
    auto player = std::make_shared<Player>(next_player_id_++, player_name, auth_token, map_id);
    
    // Добавляем игрока в сессию
    session->AddPlayer(player);
    
    // Сохраняем ссылки на игрока
    players_by_token_[auth_token] = player;
    players_by_id_[player->GetId()] = player;
    
    return player;
}

std::shared_ptr<Player> GameSessionManager::FindPlayerByToken(const std::string& auth_token) {
    auto it = players_by_token_.find(auth_token);
    return it != players_by_token_.end() ? it->second : nullptr;
}

std::shared_ptr<Player> GameSessionManager::FindPlayerById(Player::Id player_id) {
    auto it = players_by_id_.find(player_id);
    return it != players_by_id_.end() ? it->second : nullptr;
}

} // namespace model