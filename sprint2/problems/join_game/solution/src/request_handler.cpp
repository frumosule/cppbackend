#include "request_handler.h"
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace http_handler {

namespace json = boost::json;

// Обновленный конструктор
RequestHandler::RequestHandler(model::Game& game, model::GameSessionManager& session_manager, const std::string& static_path) 
    : game_(game), session_manager_(session_manager), static_root_(static_path) {
    
    // Инициализация MIME-типов
    mime_types_ = {
        {".htm", "text/html"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"},
        {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };
}

StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
    const auto target = std::string(req.target());
    
    if (target.starts_with("/api/")) {
        return HandleApiRequest(std::move(req));
    }
    
    return HandleStaticRequest(std::move(req));
}

StringResponse RequestHandler::HandleApiRequest(StringRequest&& req) {
    const auto target = std::string(req.target());
    
    // Существующие endpoints для карт
    if (req.method() == http::verb::get && target == "/api/v1/maps") {
        return GetMapsList(req);
    }
    
    if (req.method() == http::verb::get && target.starts_with("/api/v1/maps/")) {
        std::string map_id = target.substr(13);
        return GetMapById(map_id, req);
    }
    
    // Новые endpoints для игры
    if (req.method() == http::verb::post && target == "/api/v1/game/join") {
        return HandleJoinGame(std::move(req));
    }
    
    if (req.method() == http::verb::get && target == "/api/v1/game/players") {
        return HandleGetPlayers(std::move(req));
    }
    
    return MakeJsonResponse(http::status::bad_request, 
                          json::object{{"code", "badRequest"}, {"message", "Bad request"}}, req);
}

StringResponse RequestHandler::HandleJoinGame(StringRequest&& req) {
    // Проверяем метод
    if (req.method() != http::verb::post) {
        StringResponse response(http::status::method_not_allowed, req.version());
        response.set(http::field::content_type, "application/json");
        response.set(http::field::allow, "POST");
        response.set(http::field::cache_control, "no-cache");
        response.body() = json::serialize(json::object{
            {"code", "invalidMethod"},
            {"message", "Only POST method is expected"}
        });
        response.prepare_payload();
        return response;
    }
    
    // Проверяем Content-Type
    if (req.find(http::field::content_type) == req.end() || 
        req[http::field::content_type] != "application/json") {
        return MakeJsonResponse(http::status::bad_request,
                              json::object{{"code", "invalidArgument"}, {"message", "Invalid content type"}}, req);
    }
    
    try {
        // Парсим JSON
        json::value json_body = json::parse(req.body());
        json::object obj = json_body.as_object();
        
        // Проверяем обязательные поля
        if (!obj.contains("userName") || !obj.contains("mapId")) {
            return MakeJsonResponse(http::status::bad_request,
                                  json::object{{"code", "invalidArgument"}, {"message", "Join game request parse error"}}, req);
        }
        
        std::string user_name = json::value_to<std::string>(obj.at("userName"));
        std::string map_id = json::value_to<std::string>(obj.at("mapId"));
        
        // Проверяем имя игрока
        if (user_name.empty()) {
            return MakeJsonResponse(http::status::bad_request,
                                  json::object{{"code", "invalidArgument"}, {"message", "Invalid name"}}, req);
        }
        
        // Проверяем существование карты
        auto map = game_.FindMap(model::Map::Id(map_id));
        if (!map) {
            return MakeJsonResponse(http::status::not_found,
                                  json::object{{"code", "mapNotFound"}, {"message", "Map not found"}}, req);
        }
        
        // Добавляем игрока через GameSessionManager
        auto player = session_manager_.AddPlayer(model::Map::Id(map_id), user_name);
        
        // Формируем ответ - создаем объект правильно
        json::object response_json;  // ← ОБЪЯВЛЕНИЕ ПЕРЕМЕННОЙ ДОБАВЛЕНО ЗДЕСЬ
        response_json["authToken"] = player->GetAuthToken();
        response_json["playerId"] = player->GetId();
        
        return MakeJsonResponse(http::status::ok, response_json, req);
        
    } catch (const json::system_error& e) {
        // Ошибка парсинга JSON
        return MakeJsonResponse(http::status::bad_request,
                              json::object{{"code", "invalidArgument"}, {"message", "Join game request parse error"}}, req);
    } catch (const std::exception& e) {
        // Другие ошибки
        return MakeJsonResponse(http::status::bad_request,
                              json::object{{"code", "invalidArgument"}, {"message", "Join game request parse error"}}, req);
    }
}

StringResponse RequestHandler::HandleGetPlayers(StringRequest&& req) {
    // Проверяем метод
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        StringResponse response(http::status::method_not_allowed, req.version());
        response.set(http::field::content_type, "application/json");
        response.set(http::field::allow, "GET, HEAD");
        response.set(http::field::cache_control, "no-cache");
        response.body() = json::serialize(json::object{
            {"code", "invalidMethod"},
            {"message", "Invalid method"}
        });
        response.prepare_payload();
        return response;
    }
    
    // Извлекаем токен
    auto auth_token = ExtractAuthToken(req);
    if (!auth_token) {
        return MakeJsonResponse(http::status::unauthorized,
                              json::object{{"code", "invalidToken"}, {"message", "Authorization header is missing"}}, req);
    }
    
    // Находим игрока по токену через GameSessionManager
    auto player = session_manager_.FindPlayerByToken(*auth_token);
    if (!player) {
        return MakeJsonResponse(http::status::unauthorized,
                              json::object{{"code", "unknownToken"}, {"message", "Player token has not been found"}}, req);
    }
    
    // Находим сессию игрока через GameSessionManager
    auto session = session_manager_.FindSession(player->GetMapId());
    if (!session) {
        // Если сессии нет, возвращаем пустой список
        return MakeJsonResponse(http::status::ok, json::object{}, req);
    }
    
    // Формируем список игроков
    json::object players_json;
    for (const auto& [player_id, player_ptr] : session->GetPlayers()) {
        json::object player_info;
        player_info["name"] = player_ptr->GetName();
        players_json[std::to_string(player_id)] = player_info;
    }
    
    return MakeJsonResponse(http::status::ok, players_json, req);
}

std::optional<std::string> RequestHandler::ExtractAuthToken(const StringRequest& req) {
    auto auth_header = req.find(http::field::authorization);
    if (auth_header == req.end()) {
        return std::nullopt;
    }
    
    std::string auth_value = auth_header->value();
    
    // Проверяем формат "Bearer <token>"
    if (!auth_value.starts_with("Bearer ")) {
        return std::nullopt;
    }
    
    // Извлекаем токен (после "Bearer ")
    return auth_value.substr(7);
}

StringResponse RequestHandler::HandleStaticRequest(StringRequest&& req) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return MakeErrorResponse(http::status::method_not_allowed, 
                               "Method not allowed", req);
    }
    
    std::string decoded_path = UrlDecode(std::string(req.target()));
    
    // Убираем начальный слэш
    if (decoded_path.starts_with("/")) {
        decoded_path = decoded_path.substr(1);
    }
    
    // Если путь пустой, используем index.html
    if (decoded_path.empty()) {
        decoded_path = "index.html";
    }
    
    // Строим полный путь к файлу - исправляем оператор /
    fs::path file_path = fs::path(static_root_) / decoded_path;
    
    // Проверяем, что путь находится внутри корневой директории
    if (!IsPathWithinRoot(file_path, static_root_)) {
        return MakeErrorResponse(http::status::bad_request, "Invalid path", req);
    }
    
    // Если путь указывает на директорию, используем index.html
    if (fs::is_directory(file_path)) {
        file_path /= "index.html";
    }
    
    // Проверяем существование файла
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
        return MakeErrorResponse(http::status::not_found, "File not found", req);
    }
    
    return MakeStaticFileResponse(file_path, req);
}

// Остальные методы остаются без изменений...
std::string RequestHandler::UrlDecode(const std::string& url) {
    std::string result;
    result.reserve(url.length());
    
    for (size_t i = 0; i < url.length(); ++i) {
        if (url[i] == '%' && i + 2 < url.length()) {
            std::string hex = url.substr(i + 1, 2);
            char decoded_char;
            
            try {
                decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                result += decoded_char;
                i += 2;
            } catch (...) {
                result += url[i];
            }
        } else if (url[i] == '+') {
            result += ' ';
        } else {
            result += url[i];
        }
    }
    
    return result;
}

bool RequestHandler::IsPathWithinRoot(const fs::path& path, const fs::path& root) {
    auto canonical_path = fs::weakly_canonical(path);
    auto canonical_root = fs::weakly_canonical(root);
    
    auto mismatch_pair = std::mismatch(canonical_path.begin(), canonical_path.end(),
                                      canonical_root.begin(), canonical_root.end());
    
    return mismatch_pair.second == canonical_root.end();
}

std::string RequestHandler::GetMimeType(const std::string& extension) {
    std::string ext_lower = extension;
    boost::algorithm::to_lower(ext_lower);
    
    auto it = mime_types_.find(ext_lower);
    if (it != mime_types_.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

StringResponse RequestHandler::MakeStaticFileResponse(const fs::path& file_path, const StringRequest& req) {
    StringResponse response;
    
    try {
        // Открываем файл
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            return MakeErrorResponse(http::status::not_found, "File not found", req);
        }
        
        // Читаем содержимое файла
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        // Устанавливаем статус
        response.result(http::status::ok);
        response.version(req.version());
        response.set(http::field::content_type, GetMimeType(file_path.extension().string()));
        response.set(http::field::content_length, std::to_string(content.size()));
        response.set(http::field::cache_control, "no-cache");
        
        // Для GET-запроса добавляем тело, для HEAD - только заголовки
        if (req.method() == http::verb::get) {
            response.body() = content;
        }
        
    } catch (const std::exception&) {
        return MakeErrorResponse(http::status::internal_server_error, 
                               "Internal server error", req);
    }
    
    return response;
}

StringResponse RequestHandler::MakeErrorResponse(http::status status, const std::string& message, const StringRequest& req) {
    StringResponse response(status, req.version());
    response.set(http::field::content_type, "text/plain");
    response.set(http::field::cache_control, "no-cache");
    response.body() = message;
    response.prepare_payload();
    return response;
}

template <typename T>
StringResponse RequestHandler::MakeJsonResponse(http::status status, T&& data, const StringRequest& req) {
    StringResponse response(status, req.version());
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    
    std::string body = json::serialize(std::forward<T>(data));
    response.body() = body;
    response.set(http::field::content_length, std::to_string(body.size()));
    
    return response;
}

// Существующие методы для карт остаются без изменений
StringResponse RequestHandler::GetMapsList(const StringRequest& req) {
    json::array maps_array;
    for (const auto& map : game_.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_array.push_back(map_obj);
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

boost::json::value RequestHandler::SerializeMap(const model::Map& map) {
    json::object map_json;
    map_json["id"] = *map.GetId();
    map_json["name"] = map.GetName();
    
    json::array roads_array;
    for (const auto& road : map.GetRoads()) {
        roads_array.push_back(SerializeRoad(road));
    }
    map_json["roads"] = roads_array;
    
    json::array buildings_array;
    for (const auto& building : map.GetBuildings()) {
        buildings_array.push_back(SerializeBuilding(building));
    }
    map_json["buildings"] = buildings_array;
    
    json::array offices_array;
    for (const auto& office : map.GetOffices()) {
        offices_array.push_back(SerializeOffice(office));
    }
    map_json["offices"] = offices_array;
    
    return map_json;
}

boost::json::value RequestHandler::SerializeRoad(const model::Road& road) {
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

boost::json::value RequestHandler::SerializeBuilding(const model::Building& building) {
    const auto& bounds = building.GetBounds();
    json::object building_json;
    building_json["x"] = bounds.position.x;
    building_json["y"] = bounds.position.y;
    building_json["w"] = bounds.size.width;
    building_json["h"] = bounds.size.height;
    return building_json;
}

boost::json::value RequestHandler::SerializeOffice(const model::Office& office) {
    json::object office_json;
    office_json["id"] = *office.GetId();
    office_json["x"] = office.GetPosition().x;
    office_json["y"] = office.GetPosition().y;
    office_json["offsetX"] = office.GetOffset().dx;
    office_json["offsetY"] = office.GetOffset().dy;
    return office_json;
}

} // namespace http_handler