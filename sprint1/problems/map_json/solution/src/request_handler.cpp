#include "request_handler.h"

namespace http_handler {

RequestHandler::RequestHandler(model::Game& game) : game_(game) {
}

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
    
    json::object map_json;
    map_json["id"] = *map->GetId();
    map_json["name"] = map->GetName();
    
    json::array roads_array;
    for (const auto& road : map->GetRoads()) {
        json::object road_json;
        road_json["x0"] = road.GetStart().x;
        road_json["y0"] = road.GetStart().y;
        if (road.IsHorizontal()) {
            road_json["x1"] = road.GetEnd().x;
        } else {
            road_json["y1"] = road.GetEnd().y;
        }
        roads_array.push_back(road_json);
    }
    map_json["roads"] = roads_array;
    
    json::array buildings_array;
    for (const auto& building : map->GetBuildings()) {
        const auto& bounds = building.GetBounds();
        buildings_array.push_back(json::object{
            {"x", bounds.position.x},
            {"y", bounds.position.y},
            {"w", bounds.size.width},
            {"h", bounds.size.height}
        });
    }
    map_json["buildings"] = buildings_array;
    
    json::array offices_array;
    for (const auto& office : map->GetOffices()) {
        offices_array.push_back(json::object{
            {"id", *office.GetId()},
            {"x", office.GetPosition().x},
            {"y", office.GetPosition().y},
            {"offsetX", office.GetOffset().dx},
            {"offsetY", office.GetOffset().dy}
        });
    }
    map_json["offices"] = offices_array;
    
    return MakeJsonResponse(http::status::ok, map_json, req);
}

}  // namespace http_handler