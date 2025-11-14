#include "json_loader.h"

#include <fstream>

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;
    auto json_config = json::parse(ReadFileToString(json_path));
    for (const auto& parsed_map : json_config.as_object().at("maps").as_array()) {
        json::object object_map = parsed_map.as_object();
        auto parsed_id = object_map.at("id").as_string().c_str();
        model::Map model_map(model::Map::Id(parsed_id), object_map.at("name").as_string().c_str());
        for (const auto& building : object_map.at("buildings").as_array()) {
            model_map.AddBuilding(JsonToBuilding(building.as_object()));
        }
        for (const auto& road : object_map.at("roads").as_array()) {
            model_map.AddRoad(JsonToRoad(road.as_object()));
        }
        for (const auto& office : object_map.at("offices").as_array()) {
            model_map.AddOffice(JsonToOffice(office.as_object()));
        }
        game.AddMap(model_map);
    }
    return game;
}

std::string ReadFileToString(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + filePath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

model::Building JsonToBuilding(const json::object& obj) {
    model::Rectangle rect{ {obj.at("x").to_number<int>(), obj.at("y").to_number<int>()}
            , {obj.at("w").to_number<int>(), obj.at("h").to_number<int>()}};
    return model::Building(rect);
}

model::Office JsonToOffice(const json::object& obj) {
    
    return model::Office(model::Office::Id(obj.at("id").as_string().c_str())
        , { obj.at("x").to_number<int>(), obj.at("y").to_number<int>() }
        , {obj.at("offsetX").to_number<int>(), obj.at("offsetY").to_number<int>() });
}

model::Road JsonToRoad(const json::object& obj) {
    if (obj.contains("x1")) {
        return model::Road(model::Road::HORIZONTAL
            , { obj.at("x0").to_number<int>(), obj.at("y0").to_number<int>() }
            , obj.at("x1").to_number<int>());
    }
    else {
        return model::Road(model::Road::VERTICAL
            , { obj.at("x0").to_number<int>(), obj.at("y0").to_number<int>() }
            , obj.at("y1").to_number<int>());
    }
}

}  // namespace json_loader