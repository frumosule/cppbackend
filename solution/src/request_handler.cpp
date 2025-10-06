#include "request_handler.h"

namespace http_handler {

StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
    const auto target = std::string(req.target());
    
    if (req.method() == http::verb::get && target == "/api/v1/maps") {
        return GetMapsList(req);
    }
    
    if (req.method() == http::verb::get && target.starts_with("/api/v1/maps/")) {
        std::string map_id = target.substr(13);
        return GetMapById(map_id, req);
    }
    
    if (target.starts_with("/api/")) {
        return MakeJsonResponse(http::status::bad_request, 
                               json::object{{"code", "badRequest"}, {"message", "Bad request"}}, req);
    }
    
    return MakeJsonResponse(http::status::not_found,
                           json::object{{"code", "pageNotFound"}, {"message", "Page not found"}}, req);
}

StringResponse RequestHandler::GetMapsList(const StringRequest& req) {
    json::array maps_array;
    for (const auto& map : game_.GetMaps()) {
        maps_array.push_back(json::object{
            {"id", *map.GetId()},
            {"name", map.GetName()}
        });
    }
    return MakeJsonResponse(http::status::ok, maps_array, req);
}

StringResponse RequestHandler::GetMapById(const std::string& map_id, const StringRequest& req) {
    auto map = game_.FindMap(model::Map::Id(map_id));
    if (!map) {
        return MakeJsonResponse(http::status::not_found,
                               json::object{{"code", "mapNotFound"}, {"message", "Map not found"}}, req);
    }
    
    return MakeJsonResponse(http::status::ok, SerializeMap(*map), req);
}

json::value RequestHandler::SerializeMap(const model::Map& map) {
    json::object map_json;
    map_json["id"] = *map.GetId();
    map_json["name"] = map.GetName();
    
    // Сериализуем дороги
    json::array roads_array;
    for (const auto& road : map.GetRoads()) {
        roads_array.push_back(SerializeRoad(road));
    }
    map_json["roads"] = roads_array;
    
    // Сериализуем здания
    json::array buildings_array;
    for (const auto& building : map.GetBuildings()) {
        buildings_array.push_back(SerializeBuilding(building));
    }
    map_json["buildings"] = buildings_array;
    
    // Сериализуем офисы
    json::array offices_array;
    for (const auto& office : map.GetOffices()) {
        offices_array.push_back(SerializeOffice(office));
    }
    map_json["offices"] = offices_array;
    
    return map_json;
}

json::value RequestHandler::SerializeRoad(const model::Road& road) {
    json::object road_json;
    road_json["x0"] = road.GetStart().x;
    road_json["y0"] = road.GetStart().y;
    
    if (road.IsHorizontal()) {
        road_json["x1"] = road.GetEnd().x;
    } else {
        road_json["y1"] = road.GetEnd().y;
    }
    
    return road_json;
}

json::value RequestHandler::SerializeBuilding(const model::Building& building) {
    const auto& bounds = building.GetBounds();
    return json::object{
        {"x", bounds.position.x},
        {"y", bounds.position.y},
        {"w", bounds.size.width},
        {"h", bounds.size.height}
    };
}

json::value RequestHandler::SerializeOffice(const model::Office& office) {
    return json::object{
        {"id", *office.GetId()},
        {"x", office.GetPosition().x},
        {"y", office.GetPosition().y},
        {"offsetX", office.GetOffset().dx},
        {"offsetY", office.GetOffset().dy}
    };
}

template <typename T>
StringResponse RequestHandler::MakeJsonResponse(http::status status, T&& data, const StringRequest& req) {
    StringResponse response(status, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    
    std::stringstream ss;
    ss << std::forward<T>(data);
    response.body() = ss.str();
    
    response.prepare_payload();
    return response;
}

}  // namespace http_handler
