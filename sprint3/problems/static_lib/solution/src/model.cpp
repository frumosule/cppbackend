#include "model.h"

#include <stdexcept>
#include <random>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

Point Map::GetRandomPoint(std::mt19937& generator) const {
    if (roads_.empty()) {
        return {0, 0};
    }
    
    // Выбираем случайную дорогу
    std::uniform_int_distribution<size_t> road_dist(0, roads_.size() - 1);
    const Road& road = roads_[road_dist(generator)];
    
    const auto& start = road.GetStart();
    const auto& end = road.GetEnd();
    
    if (road.IsHorizontal()) {
        // Горизонтальная дорога
        int min_x = std::min(start.x, end.x);
        int max_x = std::max(start.x, end.x);
        std::uniform_real_distribution<double> x_dist(min_x + 0.4, max_x - 0.4);
        double x = x_dist(generator);
        return {static_cast<Coord>(x), start.y};
    } else {
        // Вертикальная дорога
        int min_y = std::min(start.y, end.y);
        int max_y = std::max(start.y, end.y);
        std::uniform_real_distribution<double> y_dist(min_y + 0.4, max_y - 0.4);
        double y = y_dist(generator);
        return {start.x, static_cast<Coord>(y)};
    }
}


void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

}  // namespace model
