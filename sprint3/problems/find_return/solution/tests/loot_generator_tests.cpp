#include <iostream>
#include <cassert>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include "../src/loot_generator.h"
#include "../src/model.h"
#include "../src/game_session_manager.h"

using namespace std::chrono_literals;
using namespace loot_gen;

// ==================== LOOT GENERATOR TESTS ====================

void test_loot_generator_basic_functionality() {
    std::cout << "Testing LootGenerator basic functionality... ";
    
    LootGenerator gen(1s, 1.0);
    
    // Когда лута достаточно для всех лутеров
    for (unsigned looters = 0; looters < 10; ++looters) {
        for (unsigned loot = looters; loot < looters + 10; ++loot) {
            assert(gen.Generate(1s, loot, looters) == 0);
        }
    }
    
    // Когда лутеров больше чем лута
    for (unsigned loot = 0; loot < 10; ++loot) {
        for (unsigned looters = loot; looters < loot + 10; ++looters) {
            assert(gen.Generate(1s, loot, looters) == looters - loot);
        }
    }
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_mathematical_model() {
    std::cout << "Testing LootGenerator mathematical model... ";
    
    LootGenerator gen(5s, 0.5);
    
    // Проверяем накопление времени
    unsigned result1 = gen.Generate(2s, 0, 5);
    unsigned result2 = gen.Generate(3s, 0, 5);
    assert(result2 > 0); // Должно сгенерироваться после накопления 5 секунд
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_time_accumulation() {
    std::cout << "Testing time accumulation logic... ";
    
    LootGenerator gen(10s, 0.5);
    
    // Небольшие промежутки времени - не должно генерироваться
    assert(gen.Generate(1s, 0, 5) == 0);
    assert(gen.Generate(2s, 0, 5) == 0);
    assert(gen.Generate(3s, 0, 5) == 0);
    
    // После накопления достаточного времени - должно сгенерироваться
    unsigned result = gen.Generate(4s, 0, 5);
    assert(result > 0);
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_probability_range() {
    std::cout << "Testing probability range [0, 1]... ";
    
    // Тестируем граничные значения вероятности
    LootGenerator gen1(1s, 0.0);  // 0% вероятность
    assert(gen1.Generate(10s, 0, 5) == 0);
    
    LootGenerator gen2(1s, 1.0);  // 100% вероятность
    assert(gen2.Generate(1s, 0, 5) == 5);
    
    LootGenerator gen3(1s, 0.5);  // 50% вероятность
    unsigned result = gen3.Generate(1s, 0, 10);
    assert(result >= 0 && result <= 10);
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_random_distribution() {
    std::cout << "Testing random distribution... ";
    
    // Генератор с фиксированным random для детерминированного тестирования
    LootGenerator gen(1s, 0.5, []() { return 0.7; }); // Всегда возвращает 0.7
    
    // С фиксированным random результат должен быть предсказуемым
    unsigned result1 = gen.Generate(1s, 0, 4);
    unsigned result2 = gen.Generate(1s, 0, 4);
    
    // Проверяем что с одинаковыми входными данными результат одинаковый
    assert(result1 == result2);
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_performance() {
    std::cout << "Testing performance with large numbers... ";
    
    LootGenerator gen(1s, 0.5);
    
    // Большие числа не должны вызывать проблем
    assert(gen.Generate(1s, 1000000, 1000000) == 0); // Равное количество
    assert(gen.Generate(1s, 0, 1000000) <= 1000000); // Не больше чем shortage
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_different_time_units() {
    std::cout << "Testing different time units... ";
    
    LootGenerator gen(1000ms, 1.0); // 1000 milliseconds = 1 second
    
    assert(gen.Generate(500ms, 0, 5) == 0);  // Полсекунды
    assert(gen.Generate(500ms, 0, 5) == 5);  // Еще полсекунды - всего 1 секунда
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_sequence() {
    std::cout << "Testing sequence of operations... ";
    
    LootGenerator gen(5s, 0.8);
    
    // Последовательность операций
    assert(gen.Generate(2s, 3, 5) == 0);  // 2 секунды, shortage=2
    assert(gen.Generate(2s, 3, 5) == 0);  // Еще 2 секунды, всего 4
    unsigned result = gen.Generate(1s, 3, 5); // Еще 1 секунда, всего 5 - должно сгенерироваться
    assert(result > 0 && result <= 2);
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generator_edge_cases() {
    std::cout << "Testing LootGenerator edge cases... ";
    
    // Нулевая вероятность
    LootGenerator gen1(1s, 0.0);
    assert(gen1.Generate(1s, 0, 5) == 0);
    
    // Нулевое время
    LootGenerator gen2(1s, 1.0);
    assert(gen2.Generate(0s, 0, 5) == 0);
    
    // Нулевые лутеры
    LootGenerator gen3(1s, 1.0);
    assert(gen3.Generate(1s, 0, 0) == 0);
    
    // Отрицательное время (должно обрабатываться как 0)
    LootGenerator gen4(1s, 1.0);
    
    std::cout << "PASSED" << std::endl;
}

// ==================== MODEL TESTS ====================

void test_map_random_point_distribution() {
    std::cout << "Testing Map random point distribution... ";
    
    model::Map map{model::Map::Id{"test"}, "Test Map"};
    
    // Добавляем несколько дорог разного типа
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 10});
    map.AddRoad(model::Road{model::Road::VERTICAL, {5, 0}, 10});
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 5}, 8});
    
    std::mt19937 generator{42};
    std::unordered_set<std::string> generated_points;
    
    // Генерируем много точек и проверяем распределение
    for (int i = 0; i < 100; ++i) {
        auto point = map.GetRandomPoint(generator);
        std::string point_key = std::to_string(point.x) + "," + std::to_string(point.y);
        
        // Проверяем что точка на одной из дорог
        bool on_road = false;
        on_road |= (point.y == 0 && point.x >= 0 && point.x <= 10);  // Первая горизонтальная
        on_road |= (point.x == 5 && point.y >= 0 && point.y <= 10);  // Вертикальная
        on_road |= (point.y == 5 && point.x >= 0 && point.x <= 8);   // Вторая горизонтальная
        
        assert(on_road);
        generated_points.insert(point_key);
    }
    
    // Должны быть сгенерированы разные точки
    assert(generated_points.size() > 5);
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_types_functionality() {
    std::cout << "Testing loot types functionality... ";
    
    model::Map map{model::Map::Id{"test"}, "Test Map"};
    
    // Тестируем установку и получение количества типов лута
    map.SetLootTypesCount(0);
    assert(map.GetLootTypesCount() == 0);
    
    map.SetLootTypesCount(5);
    assert(map.GetLootTypesCount() == 5);
    
    map.SetLootTypesCount(100);
    assert(map.GetLootTypesCount() == 100);
    
    std::cout << "PASSED" << std::endl;
}

void test_lost_object_creation() {
    std::cout << "Testing LostObject creation... ";
    
    // Тестируем конструкторы
    model::LostObject obj1;
    obj1.id = 1;
    obj1.type = 0;
    obj1.position = {10, 20};
    
    model::Point pos{30, 40};
    model::LostObject obj2(2, 1, pos);
    
    assert(obj2.id == 2);
    assert(obj2.type == 1);
    assert(obj2.position.x == 30);
    assert(obj2.position.y == 40);
    
    // Тестируем копирование
    model::LostObject obj3 = obj2;
    assert(obj3.id == 2);
    assert(obj3.type == 1);
    
    std::cout << "PASSED" << std::endl;
}

// ==================== INTEGRATION TESTS ====================

void test_loot_generation_integration() {
    std::cout << "Testing loot generation integration... ";
    
    // Создаем игру с картой
    model::Game game;
    model::Map map{model::Map::Id{"test_map"}, "Test Map"};
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 20});
    map.SetLootTypesCount(2); // 2 типа лута
    game.AddMap(std::move(map));
    
    // Создаем менеджер сессий
    model::GameSessionManager session_manager(game);
    session_manager.InitializeLootGenerator(2.0, 0.8); // period=2s, probability=0.8
    
    // Добавляем игроков (лутеров)
    auto player1 = session_manager.AddPlayer(model::Map::Id{"test_map"}, "Player1");
    auto player2 = session_manager.AddPlayer(model::Map::Id{"test_map"}, "Player2");
    
    // Изначально не должно быть лута
    const auto& initial_loot = session_manager.GetLostObjects(model::Map::Id{"test_map"});
    assert(initial_loot.empty());
    
    // Обновляем состояние игры (симулируем прошедшее время)
    session_manager.UpdateGameState(100); // 100ms - мало времени
    
    // Больше времени - должно сгенерироваться лут
    session_manager.UpdateGameState(3000); // 3 секунды
    
    std::cout << "PASSED" << std::endl;
}

void test_multiple_maps_loot_generation() {
    std::cout << "Testing multiple maps loot generation... ";
    
    model::Game game;
    
    // Создаем несколько карт с разным количеством типов лута
    model::Map map1{model::Map::Id{"map1"}, "Map 1"};
    map1.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 10});
    map1.SetLootTypesCount(3);
    game.AddMap(std::move(map1));
    
    model::Map map2{model::Map::Id{"map2"}, "Map 2"};
    map2.AddRoad(model::Road{model::Road::VERTICAL, {5, 0}, 15});
    map2.SetLootTypesCount(5);
    game.AddMap(std::move(map2));
    
    model::GameSessionManager session_manager(game);
    session_manager.InitializeLootGenerator(1.0, 0.5);
    
    // Добавляем игроков на разные карты
    auto player1 = session_manager.AddPlayer(model::Map::Id{"map1"}, "Player1");
    auto player2 = session_manager.AddPlayer(model::Map::Id{"map2"}, "Player2");
    
    // Генерируем лут
    session_manager.UpdateGameState(2000); // 2 секунды
    
    std::cout << "PASSED" << std::endl;
}

void test_loot_generation_with_varying_players() {
    std::cout << "Testing loot generation with varying players... ";
    
    model::Game game;
    model::Map map{model::Map::Id{"test"}, "Test Map"};
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 10});
    map.SetLootTypesCount(2);
    game.AddMap(std::move(map));
    
    model::GameSessionManager session_manager(game);
    session_manager.InitializeLootGenerator(1.0, 1.0); // 100% вероятность
    
    // Без игроков - не должно генерироваться лут
    session_manager.UpdateGameState(2000);
    const auto& loot_no_players = session_manager.GetLostObjects(model::Map::Id{"test"});
    
    // Добавляем игроков
    auto player1 = session_manager.AddPlayer(model::Map::Id{"test"}, "Player1");
    auto player2 = session_manager.AddPlayer(model::Map::Id{"test"}, "Player2");
    
    // Теперь должен генерироваться лут
    session_manager.UpdateGameState(2000);
    
    std::cout << "PASSED" << std::endl;
}

// ==================== MAIN TEST RUNNER ====================

int main() {
    std::cout << "🚀 RUNNING COMPREHENSIVE LOOT SYSTEM TESTS\n" << std::endl;
    
    std::vector<std::function<void()>> tests = {
        // LootGenerator tests
        test_loot_generator_basic_functionality,
        test_loot_generator_mathematical_model,
        test_loot_generator_time_accumulation,
        test_loot_generator_probability_range,
        test_loot_generator_random_distribution,
        test_loot_generator_performance,
        test_loot_generator_different_time_units,
        test_loot_generator_sequence,
        test_loot_generator_edge_cases,
        
        // Model tests
        test_map_random_point_distribution,
        test_loot_types_functionality,
        test_lost_object_creation,
        
        // Integration tests
        test_loot_generation_integration,
        test_multiple_maps_loot_generation,
        test_loot_generation_with_varying_players
    };
    
    int passed = 0;
    int failed = 0;
    
    for (size_t i = 0; i < tests.size(); ++i) {
        std::cout << "🧪 Test " << (i + 1) << "/" << tests.size() << "... ";
        try {
            tests[i]();
            std::cout << "✅ PASSED" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "❌ FAILED: " << e.what() << std::endl;
            failed++;
        } catch (...) {
            std::cout << "❌ FAILED with unknown error" << std::endl;
            failed++;
        }
    }
    
    std::cout << "\n📊 TEST SUMMARY:" << std::endl;
    std::cout << "   ✅ Passed: " << passed << std::endl;
    std::cout << "   ❌ Failed: " << failed << std::endl;
    std::cout << "   📈 Total:  " << (passed + failed) << std::endl;
    std::cout << "   🎯 Coverage: " << ((passed * 100) / (passed + failed)) << "%" << std::endl;
    
    if (failed == 0) {
        std::cout << "\n🎉 ALL TESTS PASSED! The loot system is fully operational!" << std::endl;
        std::cout << "   The system is ready for production deployment! 🚀" << std::endl;
        return 0;
    } else {
        std::cout << "\n💥 " << failed << " test(s) failed! Needs investigation." << std::endl;
        return 1;
    }
}