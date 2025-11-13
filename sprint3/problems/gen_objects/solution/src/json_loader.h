#pragma once

#include "model.h"
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <boost/json.hpp>

namespace json_loader {

// НОВОЕ: Структура для конфигурации генератора лута
struct LootGeneratorConfig {
    double period;
    double probability;
};

// НОВОЕ: Структура для возврата всех данных
struct GameLoadResult {
    model::Game game;
    std::optional<LootGeneratorConfig> loot_config;
    std::unordered_map<model::Map::Id, boost::json::array, model::Game::MapIdHasher> map_loot_types;
};

// Оригинальная функция для обратной совместимости
model::Game LoadGame(const std::filesystem::path& json_path);

// НОВОЕ: Расширенная функция загрузки
GameLoadResult LoadGameWithLootConfig(const std::filesystem::path& json_path);

class JsonLoaderException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

namespace detail {

std::string LoadFileContent(const std::filesystem::path& path);
boost::json::value ParseJson(const std::string& content);
model::Map ParseMap(const boost::json::object& map_obj);
model::Road ParseRoad(const boost::json::object& road_obj);
model::Building ParseBuilding(const boost::json::object& building_obj);
model::Office ParseOffice(const boost::json::object& office_obj);

} // namespace detail

} // namespace json_loader