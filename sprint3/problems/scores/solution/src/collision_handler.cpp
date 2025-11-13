#include "collision_handler.h"
#include <algorithm>

namespace model {

// Конструктор GameProvider - ДОБАВЛЯЕМ time_delta параметр
CollisionHandler::GameProvider::GameProvider(
    const std::unordered_map<Player::Id, std::shared_ptr<Player>>& players,
    const std::unordered_map<LostObject::Id, LostObject>& lost_objects,
    const std::vector<Office>& offices,
    double time_delta)  // ДОБАВЛЯЕМ параметр
    : players_(players), lost_objects_(lost_objects), offices_(offices), time_delta_(time_delta) {
    
    // Подготавливаем предметы (потерянные вещи)
    for (const auto& [id, lost_obj] : lost_objects_) {
        collision_detector::Item item;
        item.position = geom::Point2D{
            static_cast<double>(lost_obj.position.x),
            static_cast<double>(lost_obj.position.y)
        };
        item.width = 0.0; // Ширина предметов = 0
        items_.push_back(item);
    }
    
    // Подготавливаем офисы как предметы для сбора
    for (const auto& office : offices_) {
        collision_detector::Item item;
        auto pos = office.GetPosition();
        item.position = geom::Point2D{
            static_cast<double>(pos.x),
            static_cast<double>(pos.y)
        };
        item.width = 0.5; // Ширина офиса = 0.5
        items_.push_back(item);
    }
    
    // Подготавливаем собирателей (игроков)
    for (const auto& [id, player] : players_) {
        collision_detector::Gatherer gatherer;
        gatherer.start_pos = geom::Point2D{
            player->GetPosX(),
            player->GetPosY()
        };
        
        // Вычисляем конечную позицию на основе скорости и времени
        // ТЕПЕРЬ time_delta_ доступен - это поле класса
        double end_x = player->GetPosX() + player->GetSpeedX() * time_delta_;
        double end_y = player->GetPosY() + player->GetSpeedY() * time_delta_;
        
        gatherer.end_pos = geom::Point2D{end_x, end_y};
        gatherer.width = 0.6; // Ширина игрока = 0.6
        
        gatherers_.push_back(gatherer);
    }
}

size_t CollisionHandler::GameProvider::ItemsCount() const {
    return items_.size();
}

collision_detector::Item CollisionHandler::GameProvider::GetItem(size_t idx) const {
    return items_.at(idx);
}

size_t CollisionHandler::GameProvider::GatherersCount() const {
    return gatherers_.size();
}

collision_detector::Gatherer CollisionHandler::GameProvider::GetGatherer(size_t idx) const {
    return gatherers_.at(idx);
}

// Основная функция обработки коллизий
CollisionHandler::CollisionResult CollisionHandler::ProcessCollisions(
    const std::unordered_map<Player::Id, std::shared_ptr<Player>>& players,
    const std::unordered_map<LostObject::Id, LostObject>& lost_objects,
    const std::vector<Office>& offices,
    double time_delta) {
    
    CollisionResult result;
    
    if (players.empty() || (lost_objects.empty() && offices.empty())) {
        return result;
    }
    
    // Создаем провайдер - ПЕРЕДАЕМ time_delta
    GameProvider provider(players, lost_objects, offices, time_delta);
    
    // Находим события сбора
    auto events = collision_detector::FindGatherEvents(provider);
    
    // Разделяем события на сбор предметов и возврат на базу
    size_t lost_objects_count = lost_objects.size();
    
    for (const auto& event : events) {
        if (event.gatherer_id >= players.size()) continue;
        
        // Находим игрока
        auto player_it = std::next(players.begin(), event.gatherer_id);
        auto player = player_it->second;
        
        if (event.item_id < lost_objects_count) {
            // Это событие сбора потерянного предмета
            auto lost_obj_it = std::next(lost_objects.begin(), event.item_id);
            LostObject::Id obj_id = lost_obj_it->first;
            
            // Проверяем, может ли игрок взять предмет
            if (!player->IsBagFull() && !player->HasItem(obj_id)) {
                result.collected_items[player->GetId()].push_back(obj_id);
            }
        } else {
            // Это событие возврата на базу
            result.returned_to_office[player->GetId()] = true;
        }
    }
    
    return result;
}

} // namespace model