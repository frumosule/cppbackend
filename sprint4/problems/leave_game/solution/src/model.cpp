#include "model.h"
#include <algorithm>
#include <random>
#include <stdexcept>

namespace model {
using namespace std::literals;

// PointHash implementation
size_t PointHash::operator()(const Point& p) const {
    return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
}

// Road implementations
Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
    : start_{start}, end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
    : start_{start}, end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

// Building implementations
Building::Building(Rectangle bounds) noexcept
    : bounds_{bounds} {
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

// Office implementations
Office::Office(Id id, Point position, Offset offset) noexcept
    : id_{std::move(id)}, position_{position}, offset_{offset} {
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

// LostObject implementation
LostObject::LostObject(Id id, int type, Point position, int value)
    : id(id), type(type), position(position), value(value) {
}

// Map implementations
Map::Map(Id id, std::string name) noexcept
    : id_(std::move(id)), name_(std::move(name)) {
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

bool Map::HasDogSpeed() const noexcept {
    return dog_speed_.has_value();
}

double Map::GetDogSpeed() const noexcept {
    return dog_speed_.value_or(1.0);
}

void Map::SetDogSpeed(double speed) {
    dog_speed_ = speed;
}

void Map::SetLootTypesCount(size_t count) {
    loot_types_count_ = count;
}

size_t Map::GetLootTypesCount() const {
    return loot_types_count_;
}

bool Map::HasBagCapacity() const noexcept {
    return bag_capacity_.has_value();
}

size_t Map::GetBagCapacity() const noexcept {
    return bag_capacity_.value_or(0);
}

void Map::SetBagCapacity(size_t capacity) {
    bag_capacity_ = capacity;
}

Point Map::GetRandomPoint(std::mt19937& generator) const {
    if (roads_.empty()) {
        return {0, 0};
    }
    
    std::uniform_int_distribution<size_t> road_dist(0, roads_.size() - 1);
    const Road& road = roads_[road_dist(generator)];
    
    const auto& start = road.GetStart();
    const auto& end = road.GetEnd();
    static constexpr double ROAD_OFFSET = 0.4;
    
    if (road.IsHorizontal()) {
        int min_x = std::min(start.x, end.x);
        int max_x = std::max(start.x, end.x);
        std::uniform_real_distribution<double> x_dist(min_x + ROAD_OFFSET, max_x - ROAD_OFFSET);
        double x = x_dist(generator);
        return {static_cast<Coord>(x), start.y};
    } else {
        int min_y = std::min(start.y, end.y);
        int max_y = std::max(start.y, end.y);
        std::uniform_real_distribution<double> y_dist(min_y + ROAD_OFFSET, max_y - ROAD_OFFSET);
        double y = y_dist(generator);
        return {start.x, static_cast<Coord>(y)};
    }
}

// Game implementations
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

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

double Game::GetDefaultDogSpeed() const noexcept {
    return default_dog_speed_;
}

void Game::SetDefaultDogSpeed(double speed) {
    default_dog_speed_ = speed;
}

size_t Game::GetDefaultBagCapacity() const noexcept {
    return default_bag_capacity_;
}

void Game::SetDefaultBagCapacity(size_t capacity) {
    default_bag_capacity_ = capacity;
}

}  // namespace model