#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <sstream>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

using StringRequest = http_server::StringRequest;
using StringResponse = http_server::StringResponse;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game) : game_{game} {}
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto response = HandleRequest(ConvertRequest(std::move(req)));
        send(std::move(response));
    }

private:
    // Декомпозиция обработки запросов
    StringResponse HandleRequest(StringRequest&& req);
    StringResponse GetMapsList(const StringRequest& req);
    StringResponse GetMapById(const std::string& map_id, const StringRequest& req);
    
    // Декомпозиция сериализации
    json::value SerializeMap(const model::Map& map);
    json::value SerializeRoad(const model::Road& road);
    json::value SerializeBuilding(const model::Building& building);
    json::value SerializeOffice(const model::Office& office);
    
    // Вспомогательные методы
    template <typename T>
    StringResponse MakeJsonResponse(http::status status, T&& data, const StringRequest& req);
    
    // ConvertRequest должен быть шаблонным методом
    template <typename Body, typename Allocator>
    StringRequest ConvertRequest(http::request<Body, http::basic_fields<Allocator>>&& req) {
        StringRequest string_req;
        string_req.method(req.method());
        string_req.target(req.target());
        string_req.version(req.version());
        string_req.body() = req.body();
        
        for (const auto& field : req) {
            string_req.set(field.name(), field.value());
        }
        
        string_req.prepare_payload();
        return string_req;
    }

    model::Game& game_;
};

} // namespace http_handler