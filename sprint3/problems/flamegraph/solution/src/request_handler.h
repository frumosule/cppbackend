#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <sstream>  // Добавляем для std::stringstream

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

// Используем типы из http_server
using StringRequest = http_server::StringRequest;
using StringResponse = http_server::StringResponse;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Преобразуем запрос к StringRequest и обрабатываем
        StringRequest string_req;
        string_req.method(req.method());
        string_req.target(req.target());
        string_req.version(req.version());
        string_req.body() = req.body();
        string_req.prepare_payload();
        
        // Копируем заголовки
        for (const auto& field : req) {
            string_req.set(field.name(), field.value());
        }
        
        auto response = HandleRequest(std::move(string_req));
        send(std::move(response));
    }

private:
    StringResponse HandleRequest(StringRequest&& req) {
        const auto target = std::string(req.target());
        
        // Обработка запроса списка карт
        if (req.method() == http::verb::get && target == "/api/v1/maps") {
            return GetMapsList(req);
        }
        
        // Обработка запроса конкретной карты
        if (req.method() == http::verb::get && target.starts_with("/api/v1/maps/")) {
            std::string map_id = target.substr(13); // Убираем "/api/v1/maps/"
            return GetMapById(map_id, req);
        }
        
        // Обработка неизвестных API запросов
        if (target.starts_with("/api/")) {
            return MakeJsonResponse(http::status::bad_request, 
                                   json::object{{"code", "badRequest"}, {"message", "Bad request"}}, req);
        }
        
        // Для всех остальных запросов возвращаем 404
        return MakeJsonResponse(http::status::not_found,
                               json::object{{"code", "pageNotFound"}, {"message", "Page not found"}}, req);
    }

    StringResponse GetMapsList(const StringRequest& req) {
        json::array maps_array;
        for (const auto& map : game_.GetMaps()) {
            maps_array.push_back(json::object{
                {"id", *map.GetId()},
                {"name", map.GetName()}
            });
        }
        return MakeJsonResponse(http::status::ok, maps_array, req);
    }

    StringResponse GetMapById(const std::string& map_id, const StringRequest& req) {
        auto map = game_.FindMap(model::Map::Id(map_id));
        if (!map) {
            return MakeJsonResponse(http::status::not_found,
                                   json::object{{"code", "mapNotFound"}, {"message", "Map not found"}}, req);
        }
        
        json::object map_json;
        map_json["id"] = *map->GetId();
        map_json["name"] = map->GetName();
        
        // Добавляем дороги
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
        
        // Добавляем здания
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
        
        // Добавляем офисы
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

    template <typename T>
    StringResponse MakeJsonResponse(http::status status, T&& data, const StringRequest& req) {
        StringResponse response(status, req.version());
        response.set(http::field::content_type, "application/json");
        response.set(http::field::cache_control, "no-cache");
        
        // ЗАМЕНА: используем stringstream вместо json::serialize
        std::stringstream ss;
        ss << std::forward<T>(data);
        response.body() = ss.str();
        
        response.prepare_payload();
        return response;
    }

    model::Game& game_;
};

}  // namespace http_handler