#pragma once

#include "sdk.h"
#include "model.h"

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <unordered_map>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, const std::string& static_path);
    
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

private:
    StringResponse HandleRequest(StringRequest&& req);
    StringResponse HandleApiRequest(StringRequest&& req);
    StringResponse HandleStaticRequest(StringRequest&& req);
    
    // API методы
    StringResponse GetMapsList(const StringRequest& req);
    StringResponse GetMapById(const std::string& map_id, const StringRequest& req);
    
    // Сериализация
    json::value SerializeMap(const model::Map& map);
    json::value SerializeRoad(const model::Road& road);
    json::value SerializeBuilding(const model::Building& building);
    json::value SerializeOffice(const model::Office& office);
    
    // Вспомогательные методы
    template <typename T>
    StringResponse MakeJsonResponse(http::status status, T&& data, const StringRequest& req);
    
    StringResponse MakeStaticFileResponse(const fs::path& file_path, const StringRequest& req);
    StringResponse MakeErrorResponse(http::status status, const std::string& message, const StringRequest& req);
    
    // Статические файлы
    std::string GetMimeType(const std::string& extension);
    std::string UrlDecode(const std::string& url);
    bool IsPathWithinRoot(const fs::path& path, const fs::path& root);
    
    model::Game& game_;
    fs::path static_root_;
    std::unordered_map<std::string, std::string> mime_types_;
};

template <typename Body, typename Allocator, typename Send>
void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    auto response = HandleRequest(StringRequest{std::move(req)});
    return send(std::move(response));
}

}  // namespace http_handler