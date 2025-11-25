#pragma once

#include "model.h"
#include "game_session_manager.h"
#include "retired_players_manager.h"
#include <string>
#include <filesystem>
#include <unordered_map>
#include <optional>

#include <boost/beast.hpp>
#include <boost/json.hpp>

namespace http_handler
{

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace fs = std::filesystem;
    namespace json = boost::json;

    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;

    class RequestHandler
    {
    public:
        using MapLootTypes = std::unordered_map<model::Map::Id, boost::json::array, model::Game::MapIdHasher>;
        RequestHandler(
            model::Game &game,
            model::GameSessionManager &session_manager,
            const std::string &static_path,
            MapLootTypes map_loot_types = {});
        StringResponse HandleRequest(StringRequest &&req);

        void operator()(StringRequest &&req, auto &&send)
        {
            send(HandleRequest(std::move(req)));
        }

        void SetAutoTickMode(bool auto_tick) { auto_tick_mode_ = auto_tick; }
        bool IsAutoTickMode() const { return auto_tick_mode_; }

    private:
        model::Game &game_;
        model::GameSessionManager &session_manager_;
        std::shared_ptr<RetiredPlayersManager> retired_players_manager_;
        std::string static_root_;
        std::unordered_map<std::string, std::string> mime_types_;
        bool auto_tick_mode_;
        std::unordered_map<model::Map::Id, boost::json::array, model::Game::MapIdHasher> map_loot_types_;

        StringResponse HandleApiRequest(StringRequest &&req);
        StringResponse HandleStaticRequest(StringRequest &&req);

        StringResponse HandleJoinGame(StringRequest &&req);
        StringResponse HandleGetPlayers(StringRequest &&req);
        StringResponse HandlePlayerAction(StringRequest &&req);
        StringResponse HandleGameState(StringRequest &&req);
        StringResponse HandleGameTick(StringRequest &&req);       
        StringResponse HandleGetRecords(StringRequest&& req);

        static constexpr std::string_view API_V1_MAPS = "/api/v1/maps";
        static constexpr std::string_view API_V1_MAPS_PREFIX = "/api/v1/maps/";
        static constexpr std::string_view API_V1_GAME_JOIN = "/api/v1/game/join";
        static constexpr std::string_view API_V1_GAME_PLAYERS = "/api/v1/game/players";
        static constexpr std::string_view API_V1_GAME_STATE = "/api/v1/game/state";
        static constexpr std::string_view API_V1_GAME_PLAYER_ACTION = "/api/v1/game/player/action";
        static constexpr std::string_view API_V1_GAME_TICK = "/api/v1/game/tick";
        static constexpr std::string_view API_V1_GAME_RECORDS = "/api/v1/game/records";

        std::string UrlDecode(const std::string &url);
        bool IsPathWithinRoot(const fs::path &path, const fs::path &root);
        std::string GetMimeType(const std::string &extension);
        std::optional<std::string> ExtractAuthToken(const StringRequest &req);
        double GetMapDogSpeed(const model::Map &map);

        StringResponse MakeStaticFileResponse(const fs::path &file_path, const StringRequest &req);
        StringResponse MakeErrorResponse(http::status status, const std::string &message, const StringRequest &req);
        template <typename T>
        StringResponse MakeJsonResponse(http::status status, T &&data, const StringRequest &req);

        StringResponse GetMapsList(const StringRequest &req);
        StringResponse GetMapById(const std::string &map_id, const StringRequest &req);
        boost::json::value SerializeMap(const model::Map &map);
        boost::json::value SerializeRoad(const model::Road &road);
        boost::json::value SerializeBuilding(const model::Building &building);
        boost::json::value SerializeOffice(const model::Office &office);
    };

} // namespace http_handler
