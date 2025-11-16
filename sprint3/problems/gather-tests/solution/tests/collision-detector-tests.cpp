#include "../src/collision_detector.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <algorithm>
#include <sstream>

namespace collision_detector {
namespace {

using namespace geom;

// Тестовый провайдер для простых случаев
class TestProvider : public ItemGathererProvider {
public:
    TestProvider(std::vector<Item> items, std::vector<Gatherer> gatherers)
        : items_(std::move(items)), gatherers_(std::move(gatherers)) {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

// Матчер для сравнения GatheringEvent с учетом погрешности
class GatheringEventMatcher : public Catch::Matchers::MatcherBase<GatheringEvent> {
public:
    GatheringEventMatcher(size_t gatherer_id, size_t item_id, double sq_distance, double time)
        : expected_{gatherer_id, item_id, sq_distance, time} {}

    bool match(const GatheringEvent& actual) const override {
        return actual.gatherer_id == expected_.gatherer_id &&
               actual.item_id == expected_.item_id &&
               Catch::Matchers::WithinAbs(expected_.sq_distance, actual.sq_distance).match(1e-10) &&
               Catch::Matchers::WithinAbs(expected_.time, actual.time).match(1e-10);
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "equals GatheringEvent(" << expected_.gatherer_id << ", "
           << expected_.item_id << ", " << expected_.sq_distance << ", " << expected_.time << ")";
        return ss.str();
    }

private:
    GatheringEvent expected_;
};

GatheringEventMatcher EqualsEvent(size_t gatherer_id, size_t item_id, 
                                 double sq_distance, double time) {
    return GatheringEventMatcher(gatherer_id, item_id, sq_distance, time);
}

}  // namespace
}  // namespace collision_detector

// StringMaker specialization must be in the Catch namespace
namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id 
            << "," << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
}  // namespace Catch

using namespace collision_detector;
using namespace geom;

// ТЕСТ 1: Простое столкновение по горизонтали
TEST_CASE("Simple horizontal collision", "[collision_detector]") {
    // Собиратель движется горизонтально от (0,0) до (10,0) с шириной 1.0
    // Предмет находится в точке (5,0) с шириной 0.5
    // Расстояние между центрами: 0, сумма радиусов: 1.0 + 0.5 = 1.5
    // Квадрат расстояния: 0, время: 0.5 (середина пути)
    
    std::vector<Item> items = {Item{Point2D{5.0, 0.0}, 1.0}};  // width = диаметр = 1.0
    std::vector<Gatherer> gatherers = {Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 0.0}, 2.0}}; // width = диаметр = 2.0
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0], EqualsEvent(0, 0, 0.0, 0.5));
}

// ТЕСТ 2: Столкновение по вертикали
TEST_CASE("Simple vertical collision", "[collision_detector]") {
    // Собиратель движется вертикально от (0,0) до (0,10) 
    // Предмет находится в точке (0,3)
    
    std::vector<Item> items = {Item{Point2D{0.0, 3.0}, 1.0}};
    std::vector<Gatherer> gatherers = {Gatherer{Point2D{0.0, 0.0}, Point2D{0.0, 10.0}, 1.0}};
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0], EqualsEvent(0, 0, 0.0, 0.3));
}

// ТЕСТ 3: Отсутствие столкновения (предмет слишком далеко)
TEST_CASE("No collision when item is too far", "[collision_detector]") {
    // Собиратель движется от (0,0) до (10,0)
    // Предмет находится в точке (5,10) - слишком далеко по Y
    
    std::vector<Item> items = {Item{Point2D{5.0, 10.0}, 1.0}};
    std::vector<Gatherer> gatherers = {Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 0.0}, 1.0}};
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

// ТЕСТ 4: Отсутствие столкновения (предмет вне отрезка пути)
TEST_CASE("No collision when item is outside path segment", "[collision_detector]") {
    // Собиратель движется от (0,0) до (10,0)
    // Предмет находится в точке (15,0) - за пределами отрезка
    
    std::vector<Item> items = {Item{Point2D{15.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 0.0}, 1.0}};
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

// ТЕСТ 5: Несколько собирателей и предметов
TEST_CASE("Multiple gatherers and items", "[collision_detector]") {
    std::vector<Item> items = {
        Item{Point2D{2.0, 0.0}, 1.0},  // item0
        Item{Point2D{6.0, 0.0}, 1.0},  // item1
        Item{Point2D{4.0, 5.0}, 1.0}   // item2 (далеко)
    };
    
    std::vector<Gatherer> gatherers = {
        Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 0.0}, 1.0},  // gatherer0
        Gatherer{Point2D{0.0, 0.0}, Point2D{0.0, 10.0}, 1.0}   // gatherer1
    };
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    // gatherer0 должен собрать item0 и item1
    // gatherer1 не должен ничего собрать (предметы не на его пути)
    REQUIRE(events.size() == 2);
    
    // Проверяем что события в хронологическом порядке
    REQUIRE(events[0].time <= events[1].time);
    
    // Проверяем конкретные события
    bool found_event1 = false, found_event2 = false;
    for (const auto& event : events) {
        if (event.gatherer_id == 0 && event.item_id == 0) {
            CHECK_THAT(event, EqualsEvent(0, 0, 0.0, 0.2));
            found_event1 = true;
        }
        if (event.gatherer_id == 0 && event.item_id == 1) {
            CHECK_THAT(event, EqualsEvent(0, 1, 0.0, 0.6));
            found_event2 = true;
        }
    }
    REQUIRE(found_event1);
    REQUIRE(found_event2);
}

// ТЕСТ 6: Собиратель не двигается - нет столкновений
TEST_CASE("Stationary gatherer has no collisions", "[collision_detector]") {
    std::vector<Item> items = {Item{Point2D{0.0, 0.0}, 1.0}};
    std::vector<Gatherer> gatherers = {Gatherer{Point2D{0.0, 0.0}, Point2D{0.0, 0.0}, 1.0}};
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.empty());
}

// ТЕСТ 7: Диагональное движение со столкновением
TEST_CASE("Diagonal movement with collision", "[collision_detector]") {
    // Собиратель движется от (0,0) до (10,10)
    // Предмет находится в точке (5,5) - прямо на пути
    
    std::vector<Item> items = {Item{Point2D{5.0, 5.0}, 1.0}};
    std::vector<Gatherer> gatherers = {Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 10.0}, 1.0}};
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    // Время должно быть примерно 0.5 (середина пути)
    CHECK_THAT(events[0].time, Catch::Matchers::WithinAbs(0.5, 1e-10));
    // Квадрат расстояния должен быть 0 (прямо на пути)
    CHECK_THAT(events[0].sq_distance, Catch::Matchers::WithinAbs(0.0, 1e-10));
}

// ТЕСТ 8: Столкновение с учетом размеров объектов
TEST_CASE("Collision considering object sizes", "[collision_detector]") {
    // Собиратель движется от (0,0) до (10,0) с шириной 2.0 (радиус 1.0)
    // Предмет находится в точке (5,1.5) с шириной 1.0 (радиус 0.5)
    // Расстояние между центрами: 1.5, сумма радиусов: 1.0 + 0.5 = 1.5
    // Должно быть столкновение
    
    std::vector<Item> items = {Item{Point2D{5.0, 1.5}, 1.0}};
    std::vector<Gatherer> gatherers = {Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 0.0}, 2.0}};
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0].time, Catch::Matchers::WithinAbs(0.5, 1e-10));
}

// ТЕСТ 9: Порядок событий по времени
TEST_CASE("Events are in chronological order", "[collision_detector]") {
    std::vector<Item> items = {
        Item{Point2D{1.0, 0.0}, 1.0},  // item0 - раньше
        Item{Point2D{9.0, 0.0}, 1.0}   // item1 - позже
    };
    
    std::vector<Gatherer> gatherers = {
        Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 0.0}, 1.0}
    };
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.size() == 2);
    // Проверяем хронологический порядок
    REQUIRE(events[0].time < events[1].time);
    REQUIRE(events[0].item_id == 0);  // item0 должен быть первым
    REQUIRE(events[1].item_id == 1);  // item1 должен быть вторым
}

// ТЕСТ 10: Один собиратель может собрать несколько предметов
TEST_CASE("One gatherer can collect multiple items", "[collision_detector]") {
    std::vector<Item> items = {
        Item{Point2D{2.0, 0.0}, 1.0},
        Item{Point2D{4.0, 0.0}, 1.0},
        Item{Point2D{6.0, 0.0}, 1.0}
    };
    
    std::vector<Gatherer> gatherers = {
        Gatherer{Point2D{0.0, 0.0}, Point2D{10.0, 0.0}, 1.0}
    };
    
    TestProvider provider(items, gatherers);
    auto events = FindGatherEvents(provider);
    
    REQUIRE(events.size() == 3);
    
    // Проверяем что все события для одного собирателя
    for (const auto& event : events) {
        REQUIRE(event.gatherer_id == 0);
    }
    
    // Проверяем хронологический порядок
    for (size_t i = 1; i < events.size(); ++i) {
        REQUIRE(events[i-1].time <= events[i].time);
    }
}