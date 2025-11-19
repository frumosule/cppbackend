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
        size_t operator()(const Point &p) const;
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

        Road(HorizontalTag, Point start, Coord end_x) noexcept;
        Road(VerticalTag, Point start, Coord end_y) noexcept;

        bool IsHorizontal() const noexcept;
        bool IsVertical() const noexcept;
        Point GetStart() const noexcept;
        Point GetEnd() const noexcept;

    private:
        Point start_;
        Point end_;
    };

    class Building
    {
    public:
        explicit Building(Rectangle bounds) noexcept;
        const Rectangle &GetBounds() const noexcept;

    private:
        Rectangle bounds_;
    };

    class Office
    {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept;
        const Id &GetId() const noexcept;
        Point GetPosition() const noexcept;
        Offset GetOffset() const noexcept;

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    struct LostObject
    {
        using Id = size_t;
        Id id;
        int type;
        Point position;
        int value = 0;

        LostObject() = default;
        LostObject(Id id, int type, Point position, int value = 0);
    };

    class Map
    {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept;
        const Id &GetId() const noexcept;
        const std::string &GetName() const noexcept;
        const Buildings &GetBuildings() const noexcept;
        const Roads &GetRoads() const noexcept;
        const Offices &GetOffices() const noexcept;

        void AddRoad(const Road &road);
        void AddBuilding(const Building &building);
        void AddOffice(Office office);

        bool HasDogSpeed() const noexcept;
        double GetDogSpeed() const noexcept;
        void SetDogSpeed(double speed);

        void SetLootTypesCount(size_t count);
        size_t GetLootTypesCount() const;

        bool HasBagCapacity() const noexcept;
        size_t GetBagCapacity() const noexcept;
        void SetBagCapacity(size_t capacity);

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
        std::optional<size_t> bag_capacity_;
    };

    class Game
    {
    public:
        using Maps = std::vector<Map>;
        using MapIdHasher = util::TaggedHasher<Map::Id>;

        void AddMap(Map map);
        const Maps &GetMaps() const noexcept;
        const Map *FindMap(const Map::Id &id) const noexcept;

        double GetDefaultDogSpeed() const noexcept;
        void SetDefaultDogSpeed(double speed);
        size_t GetDefaultBagCapacity() const noexcept;
        void SetDefaultBagCapacity(size_t capacity);

    private:
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        double default_dog_speed_ = 1.0;
        size_t default_bag_capacity_ = 3;
    };

} // namespace model
