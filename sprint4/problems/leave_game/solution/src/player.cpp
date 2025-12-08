#include "player.h"
#include <algorithm>

namespace model {

    BagItem::BagItem(LostObject::Id id, int type, int value)
        : id(id), type(type), value(value) {
    }

    bool BagItem::operator==(const BagItem& other) const {
        return id == other.id && type == other.type;
    }

    Player::Player(Id id, std::string name, std::string auth_token, 
                   Map::Id map_id, double start_x, double start_y, size_t bag_capacity)
        : id_(id), name_(std::move(name)), auth_token_(std::move(auth_token)),
          map_id_(std::move(map_id)), pos_x_(start_x), pos_y_(start_y),
          speed_x_(0.0), speed_y_(0.0), direction_(Direction::NORTH),
          bag_capacity_(bag_capacity), score_(0) {
    }

    Player::Id Player::GetId() const noexcept {
        return id_;
    }

    const std::string& Player::GetName() const noexcept {
        return name_;
    }

    const std::string& Player::GetAuthToken() const noexcept {
        return auth_token_;
    }

    const Map::Id& Player::GetMapId() const noexcept {
        return map_id_;
    }

    double Player::GetPosX() const noexcept {
        return pos_x_;
    }

    double Player::GetPosY() const noexcept {
        return pos_y_;
    }

    void Player::SetPosition(double x, double y) {
        pos_x_ = x;
        pos_y_ = y;
    }

    double Player::GetSpeedX() const noexcept {
        return speed_x_;
    }

    double Player::GetSpeedY() const noexcept {
        return speed_y_;
    }

    void Player::SetSpeed(double speed_x, double speed_y) {
        speed_x_ = speed_x;
        speed_y_ = speed_y;
    }

    Direction Player::GetDirection() const noexcept {
        return direction_;
    }

    void Player::SetDirection(Direction dir) {
        direction_ = dir;
    }

    char Player::GetDirectionChar() const noexcept {
        return static_cast<char>(direction_);
    }

    const std::vector<BagItem>& Player::GetBag() const noexcept {
        return bag_;
    }

    size_t Player::GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    size_t Player::GetBagSize() const noexcept {
        return bag_.size();
    }

    bool Player::IsBagFull() const noexcept {
        return bag_.size() >= bag_capacity_;
    }

    void Player::SetBagCapacity(size_t capacity) {
        bag_capacity_ = capacity;
    }

    bool Player::AddToBag(LostObject::Id id, int type, int value) {
        if (IsBagFull()) {
            return false;
        }
        for (const auto& item : bag_) {
            if (item.id == id) {
                return false; 
            }
        }
        bag_.emplace_back(id, type, value);
        return true;
    }

    void Player::ClearBag() {
        bag_.clear();
    }

    bool Player::HasItem(LostObject::Id id) const {
        for (const auto& item : bag_) {
            if (item.id == id) {
                return true;
            }
        }
        return false;
    }

    bool Player::RemoveFromBag(LostObject::Id id) {
        for (auto it = bag_.begin(); it != bag_.end(); ++it) {
            if (it->id == id) {
                bag_.erase(it);
                return true;
            }
        }
        return false;
    }

    int Player::GetScore() const noexcept {
        return score_;
    }

    void Player::AddScore(int points) {
        score_ += points;
    }

    void Player::SetScore(int score) {
        score_ = score;
    }

    GameSession::GameSession(Map::Id map_id) 
        : map_id_(std::move(map_id)) {
    }

    const Map::Id& GameSession::GetId() const noexcept {
        return map_id_;
    }

    void GameSession::AddPlayer(std::shared_ptr<Player> player) {
        players_[player->GetId()] = player;
    }

    void GameSession::RemovePlayer(Player::Id player_id) {
        players_.erase(player_id);
    }

    const std::unordered_map<Player::Id, std::shared_ptr<Player>>& GameSession::GetPlayers() const noexcept {
        return players_;
    }

    std::shared_ptr<Player> GameSession::FindPlayer(Player::Id player_id) const {
        auto it = players_.find(player_id);
        return it != players_.end() ? it->second : nullptr;
    }

    bool GameSession::HasPlayer(Player::Id player_id) const {
        return players_.find(player_id) != players_.end();
    }

} // namespace model