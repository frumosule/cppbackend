#include "request_handler.h"

#include <ranges>

namespace http_handler {

std::vector<std::string_view> SplitRequest(std::string_view body) {
    std::vector<std::string_view> result;
    size_t start = 0;
    size_t end = body.find("/");
    while (end != std::string_view::npos) {
        result.push_back(body.substr(start, end - start));
        start = end + 1;
        end = body.find("/", start);
    }
    result.push_back(body.substr(start, body.length() - start));
    return result;
}

json::object MapToJSON(const model::Map* map) {
    json::object obj;
    obj["id"] = *map->GetId();
    obj["name"] = map->GetName();
    json::array roads;
    for (const auto& road : map->GetRoads()) {
        json::object json_road;
        json_road["x0"] = road.GetStart().x;
        json_road["y0"] = road.GetStart().y;
        road.IsHorizontal() ? json_road["x1"] = road.GetEnd().x : json_road["y1"] = road.GetEnd().y;
        roads.emplace_back(json_road);
    }
    obj.emplace("roads", std::move(roads));
    json::array buildings;
    for (const auto& building : map->GetBuildings()) {
        json::object json_building;
        auto& bounds = building.GetBounds();
        json_building["x"] = bounds.position.x;
        json_building["y"] = bounds.position.y;
        json_building["w"] = bounds.size.width;
        json_building["h"] = bounds.size.height;
        buildings.emplace_back(json_building);
    }
    obj.emplace("buildings", std::move(buildings));
    json::array offices;
    for (const auto& office : map->GetOffices()) {
        json::object json_office;
        json_office["id"] = *office.GetId();
        json_office["x"] = office.GetPosition().x;
        json_office["y"] = office.GetPosition().y;
        json_office["offsetX"] = office.GetOffset().dx;
        json_office["offsetY"] = office.GetOffset().dy;
        offices.emplace_back(json_office);
    }
    obj.emplace("offices", std::move(offices));
    return obj;
}

}  // namespace http_handler