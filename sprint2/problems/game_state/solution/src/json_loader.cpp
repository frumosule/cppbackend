#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    try {
        auto content = detail::LoadFileContent(json_path);
        auto json_value = detail::ParseJson(content);
        auto root = json_value.as_object();
        
        model::Game game;

        if (root.contains("defaultDogSpeed")) {
            double default_speed = root.at("defaultDogSpeed").as_double();
            game.SetDefaultDogSpeed(default_speed);
        }
        
        auto maps = root.at("maps").as_array();
        for (const auto& map_json : maps) {
            game.AddMap(detail::ParseMap(map_json.as_object()));
        }
        
        return game;
    } catch (const std::exception& e) {
        throw JsonLoaderException(std::string("Failed to load game: ") + e.what());
    }
}

namespace detail {

std::string LoadFileContent(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

json::value ParseJson(const std::string& content) {
    try {
        return json::parse(content);
    } catch (const json::system_error& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }
}

model::Map ParseMap(const json::object& map_obj) {
    std::string id = json::value_to<std::string>(map_obj.at("id"));
    std::string name = json::value_to<std::string>(map_obj.at("name"));
    
    model::Map map(model::Map::Id(id), name);

    if (map_obj.contains("dogSpeed")) {
        double dog_speed = map_obj.at("dogSpeed").as_double();
        map.SetDogSpeed(dog_speed);
    }
    
    // Загружаем дороги
    auto roads = map_obj.at("roads").as_array();
    for (const auto& road_json : roads) {
        map.AddRoad(ParseRoad(road_json.as_object()));
    }
    
    // Загружаем здания
    auto buildings = map_obj.at("buildings").as_array();
    for (const auto& building_json : buildings) {
        map.AddBuilding(ParseBuilding(building_json.as_object()));
    }
    
    // Загружаем офисы
    auto offices = map_obj.at("offices").as_array();
    for (const auto& office_json : offices) {
        map.AddOffice(ParseOffice(office_json.as_object()));
    }
    
    return map;
}

model::Road ParseRoad(const json::object& road_obj) {
    model::Coord x0 = road_obj.at("x0").as_int64();
    model::Coord y0 = road_obj.at("y0").as_int64();
    
    if (road_obj.contains("x1")) {
        // Горизонтальная дорога
        model::Coord x1 = road_obj.at("x1").as_int64();
        return model::Road(model::Road::HORIZONTAL, model::Point{x0, y0}, x1);
    } else {
        // Вертикальная дорога
        model::Coord y1 = road_obj.at("y1").as_int64();
        return model::Road(model::Road::VERTICAL, model::Point{x0, y0}, y1);
    }
}

model::Building ParseBuilding(const json::object& building_obj) {
    model::Coord x = building_obj.at("x").as_int64();
    model::Coord y = building_obj.at("y").as_int64();
    model::Coord w = building_obj.at("w").as_int64();
    model::Coord h = building_obj.at("h").as_int64();
    
    return model::Building(model::Rectangle{model::Point{x, y}, model::Size{w, h}});
}

model::Office ParseOffice(const json::object& office_obj) {
    std::string id = json::value_to<std::string>(office_obj.at("id"));
    model::Coord x = office_obj.at("x").as_int64();
    model::Coord y = office_obj.at("y").as_int64();
    model::Coord offsetX = office_obj.at("offsetX").as_int64();
    model::Coord offsetY = office_obj.at("offsetY").as_int64();
    
    return model::Office(model::Office::Id(id), model::Point{x, y}, model::Offset{offsetX, offsetY});
}

} // namespace detail
} // namespace json_loader