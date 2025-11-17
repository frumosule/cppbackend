#pragma once

#include "collision_detector.h"
#include "geom.h"
#include "model.h"
#include "player.h"
#include <memory>
#include <unordered_map>

namespace model {

class CollisionHandler {
public:
    class GameProvider : public collision_detector::ItemGathererProvider {
    public:
        GameProvider(const std::unordered_map<Player::Id, std::shared_ptr<Player>>& players,
                    const std::unordered_map<LostObject::Id, LostObject>& lost_objects,
                    const std::vector<Office>& offices,
                    double time_delta);  
        
        size_t ItemsCount() const override;
        collision_detector::Item GetItem(size_t idx) const override;
        size_t GatherersCount() const;
        collision_detector::Gatherer GetGatherer(size_t idx) const;

    private:
        const std::unordered_map<Player::Id, std::shared_ptr<Player>>& players_;
        const std::unordered_map<LostObject::Id, LostObject>& lost_objects_;
        const std::vector<Office>& offices_;
        double time_delta_;  
        std::vector<collision_detector::Item> items_;
        std::vector<collision_detector::Gatherer> gatherers_;
    };

    struct CollisionResult {
        std::unordered_map<Player::Id, std::vector<LostObject::Id>> collected_items;
        std::unordered_map<Player::Id, bool> returned_to_office;
    };

    static CollisionResult ProcessCollisions(
        const std::unordered_map<Player::Id, std::shared_ptr<Player>>& players,
        const std::unordered_map<LostObject::Id, LostObject>& lost_objects,
        const std::vector<Office>& offices,
        double time_delta);
};

} // namespace model