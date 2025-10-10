#pragma once

#include <string>
#include <memory>
#include "model.h"

namespace model {

class Player {
public:
    using Id = size_t;
    
    Player(Id id, std::string name, std::string auth_token, Map::Id map_id)
        : id_(id), name_(std::move(name)), auth_token_(std::move(auth_token)), map_id_(std::move(map_id)) {}
    
    Id GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const std::string& GetAuthToken() const noexcept { return auth_token_; }
    const Map::Id& GetMapId() const noexcept { return map_id_; }

private:
    Id id_;
    std::string name_;
    std::string auth_token_;
    Map::Id map_id_;
};

class GameSession {
public:
    using Id = Map::Id;
    
    explicit GameSession(Map::Id map_id) : map_id_(std::move(map_id)) {}
    
    const Map::Id& GetId() const noexcept { return map_id_; }
    
    void AddPlayer(std::shared_ptr<Player> player) {
        players_[player->GetId()] = player;
    }
    
    void RemovePlayer(Player::Id player_id) {
        players_.erase(player_id);
    }
    
    const std::unordered_map<Player::Id, std::shared_ptr<Player>>& GetPlayers() const noexcept {
        return players_;
    }
    
    bool HasPlayer(Player::Id player_id) const {
        return players_.find(player_id) != players_.end();
    }

private:
    Map::Id map_id_;
    std::unordered_map<Player::Id, std::shared_ptr<Player>> players_;
};

} // namespace model