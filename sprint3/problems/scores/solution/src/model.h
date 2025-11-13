#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <random>

#include "tagged.h"

namespace model
{

    using Dimension = int;
    using Coord = Dimension;

    struct Point
    {
        Coord x, y;
        bool operator==(const Point &other) const = default;
    };

    struct PointHash
    {
        size_t operator()(const Point &p) const
        {
            return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
        }
    };

    struct Size
    {
        Dimension width, height;
    };

    struct Rectangle
    {
        Point position;
        Size size;
    };

    struct Offset
    {
        Dimension dx, dy;
    };

    class Road
    {
        struct HorizontalTag
        {
            explicit HorizontalTag() = default;
        };

        struct VerticalTag
        {
            explicit VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{start}, end_{end_x, start.y}
        {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{start}, end_{start.x, end_y}
        {
        }

        bool IsHorizontal() const noexcept
        {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept
        {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept
        {
            return start_;
        }

        Point GetEnd() const noexcept
        {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building
    {
    public:
        explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds}
        {
        }

        const Rectangle &GetBounds() const noexcept
        {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office
    {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}, position_{position}, offset_{offset}
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        Point GetPosition() const noexcept
        {
            return position_;
        }

        Offset GetOffset() const noexcept
        {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    struct LostObject
    {
        using Id = size_t;
        Id id;
        int type; // от 0 до K-1, где K - количество типов трофеев на карте
        Point position;
        int value = 0; // НОВОЕ: стоимость предмета

        // ИСПРАВЛЕНО: Добавляем конструктор
        LostObject() = default;
        LostObject(Id id, int type, Point position, int value = 0)
            : id(id), type(type), position(position), value(value) {}
    };

    class Map
    {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept
            : id_(std::move(id)), name_(std::move(name))
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        const std::string &GetName() const noexcept
        {
            return name_;
        }

        const Buildings &GetBuildings() const noexcept
        {
            return buildings_;
        }

        const Roads &GetRoads() const noexcept
        {
            return roads_;
        }

        const Offices &GetOffices() const noexcept
        {
            return offices_;
        }

        void AddRoad(const Road &road)
        {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building &building)
        {
            buildings_.emplace_back(building);
        }

        void AddOffice(Office office);

        bool HasDogSpeed() const noexcept { return dog_speed_.has_value(); }
        double GetDogSpeed() const noexcept { return dog_speed_.value_or(1.0); }
        void SetDogSpeed(double speed) { dog_speed_ = speed; }

        void SetLootTypesCount(size_t count) { loot_types_count_ = count; }
        size_t GetLootTypesCount() const { return loot_types_count_; }

        bool HasBagCapacity() const noexcept { return bag_capacity_.has_value(); }
        size_t GetBagCapacity() const noexcept { return bag_capacity_.value_or(0); }
        void SetBagCapacity(size_t capacity) { bag_capacity_ = capacity; }

        // НОВОЕ: получение случайной точки на дорогах карты
        Point GetRandomPoint(std::mt19937 &generator) const;

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;

        std::optional<double> dog_speed_;
        size_t loot_types_count_ = 0;
        std::optional<size_t> bag_capacity_; // НОВОЕ
    };

    class Game
    {
    public:
        using Maps = std::vector<Map>;
        using MapIdHasher = util::TaggedHasher<Map::Id>;

        void AddMap(Map map);

        const Maps &GetMaps() const noexcept
        {
            return maps_;
        }

        const Map *FindMap(const Map::Id &id) const noexcept
        {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end())
            {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }
        void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }

        size_t GetDefaultBagCapacity() const noexcept { return default_bag_capacity_; }
        void SetDefaultBagCapacity(size_t capacity) { default_bag_capacity_ = capacity; }

    private:
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;

        double default_dog_speed_ = 1.0;
        size_t default_bag_capacity_ = 3;
    };

} // namespace model
