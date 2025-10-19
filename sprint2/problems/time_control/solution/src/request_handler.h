#pragma once

#include "model.h"
#include "game_session_manager.h"
#include <string>
#include <filesystem>
#include <unordered_map>
#include <optional>

#include <boost/beast.hpp>
#include <boost/json.hpp>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace json = boost::json;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, model::GameSessionManager& session_manager, const std::string& static_path);
    
    StringResponse HandleRequest(StringRequest&& req);

    void operator()(StringRequest&& req, auto&& send) {
        send(HandleRequest(std::move(req)));
    }


private:
    model::Game& game_;
    model::GameSessionManager& session_manager_;
    std::string static_root_;
    std::unordered_map<std::string, std::string> mime_types_;
    
    // Обработчики запросов
    StringResponse HandleApiRequest(StringRequest&& req);
    StringResponse HandleStaticRequest(StringRequest&& req);
    
    // API endpoints
    StringResponse HandleJoinGame(StringRequest&& req);
    StringResponse HandleGetPlayers(StringRequest&& req);
    StringResponse HandlePlayerAction(StringRequest&& req);
    StringResponse HandleGameState(StringRequest&& req);
    StringResponse HandleGameTick(StringRequest &&req);
    
    // Вспомогательные методы
    std::string UrlDecode(const std::string& url);
    bool IsPathWithinRoot(const fs::path& path, const fs::path& root);
    std::string GetMimeType(const std::string& extension);
    std::optional<std::string> ExtractAuthToken(const StringRequest& req);
    double GetMapDogSpeed(const model::Map& map);
    
    // Создание ответов
    StringResponse MakeStaticFileResponse(const fs::path& file_path, const StringRequest& req);
    StringResponse MakeErrorResponse(http::status status, const std::string& message, const StringRequest& req);
    template <typename T>
    StringResponse MakeJsonResponse(http::status status, T&& data, const StringRequest& req);
    
    // Существующие методы для карт
    StringResponse GetMapsList(const StringRequest& req);
    StringResponse GetMapById(const std::string& map_id, const StringRequest& req);
    boost::json::value SerializeMap(const model::Map& map);
    boost::json::value SerializeRoad(const model::Road& road);
    boost::json::value SerializeBuilding(const model::Building& building);
    boost::json::value SerializeOffice(const model::Office& office);
};

} // namespace http_handler
