#pragma once

#include "player.h"
#include <random>
#include <unordered_map>

namespace model {

class GameSessionManager {
public:
    std::shared_ptr<GameSession> FindSession(const Map::Id& map_id);
    std::shared_ptr<GameSession> CreateSession(const Map::Id& map_id);
    std::shared_ptr<Player> AddPlayer(const Map::Id& map_id, const std::string& player_name);
    std::shared_ptr<Player> FindPlayerByToken(const std::string& auth_token);
    std::shared_ptr<Player> FindPlayerById(Player::Id player_id);

private:
    std::string GenerateAuthToken();
    
    std::unordered_map<Map::Id, std::shared_ptr<GameSession>> sessions_;
    std::unordered_map<std::string, std::shared_ptr<Player>> players_by_token_;
    std::unordered_map<Player::Id, std::shared_ptr<Player>> players_by_id_;
    Player::Id next_player_id_ = 0;
    
    std::random_device rd_;
    std::mt19937_64 generator_{rd_()};
    std::uniform_int_distribution<uint64_t> distribution_;
};

} // namespace model