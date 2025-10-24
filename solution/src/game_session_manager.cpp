#include "game_session_manager.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <cmath>

namespace model
{

    std::shared_ptr<GameSession> GameSessionManager::FindSession(const Map::Id &map_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return FindSessionUnsafe(map_id);
    }

    std::string GameSessionManager::GenerateAuthToken()
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << generator1_()
           << std::hex << std::setfill('0') << std::setw(16) << generator2_();
        return ss.str();
    }

    std::shared_ptr<Player> GameSessionManager::AddPlayer(const Map::Id &map_id, const std::string &player_name)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto session = FindSessionUnsafe(map_id);
        if (!session)
        {
            session = CreateSessionUnsafe(map_id);
        }

        auto [pos_x, pos_y] = GetRandomPositionOnRoad(map_id);

        std::string auth_token = GenerateAuthToken();
        auto player = std::make_shared<Player>(next_player_id_++, player_name, auth_token, map_id, pos_x, pos_y);

        session->AddPlayer(player);
        players_by_token_[auth_token] = player;
        players_by_id_[player->GetId()] = player;

        return player;
    }

    std::shared_ptr<Player> GameSessionManager::FindPlayerByToken(const std::string &auth_token)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return FindPlayerByTokenUnsafe(auth_token);
    }

    std::shared_ptr<Player> GameSessionManager::FindPlayerById(Player::Id player_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = players_by_id_.find(player_id);
        return it != players_by_id_.end() ? it->second : nullptr;
    }

    bool GameSessionManager::UpdatePlayerAction(const std::string &auth_token, const std::string &move)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto player = FindPlayerByTokenUnsafe(auth_token);
        if (!player)
        {
            return false;
        }

        auto map = game_.FindMap(player->GetMapId());
        if (!map)
        {
            return false;
        }

        double dog_speed = map->HasDogSpeed() ? map->GetDogSpeed() : game_.GetDefaultDogSpeed();

        if (move == "L")
        {
            player->SetSpeed(-dog_speed, 0.0);
            player->SetDirection(Direction::WEST);
        }
        else if (move == "R")
        {
            player->SetSpeed(dog_speed, 0.0);
            player->SetDirection(Direction::EAST);
        }
        else if (move == "U")
        {
            player->SetSpeed(0.0, -dog_speed);
            player->SetDirection(Direction::NORTH);
        }
        else if (move == "D")
        {
            player->SetSpeed(0.0, dog_speed);
            player->SetDirection(Direction::SOUTH);
        }
        else if (move == "")
        {
            player->SetSpeed(0.0, 0.0);
        }

        return true;
    }

    std::unordered_map<Player::Id, std::shared_ptr<Player>> GameSessionManager::GetPlayersByToken(const std::string &auth_token)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto current_player = FindPlayerByTokenUnsafe(auth_token);
        if (!current_player)
        {
            return {};
        }

        auto session = FindSessionUnsafe(current_player->GetMapId());
        if (!session)
        {
            return {};
        }

        return session->GetPlayers();
    }

    std::pair<double, double> GameSessionManager::GetRandomPositionOnRoad(const Map::Id& map_id) {
    auto map = game_.FindMap(map_id);
    if (!map) {
        throw std::runtime_error("Map not found for getting random position");
    }
    
    const auto& roads = map->GetRoads();
    if (roads.empty()) {
        throw std::runtime_error("No roads on the map");
    }

    if (randomize_spawn_) {
        std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
        const auto& road = roads[road_dist(generator_)];
        
        const auto& start = road.GetStart();
        const auto& end = road.GetEnd();
        
        if (road.IsHorizontal()) {
            int min_x = std::min(start.x, end.x);
            int max_x = std::max(start.x, end.x);
            std::uniform_real_distribution<double> pos_dist(min_x + 0.4, max_x - 0.4);
            double x = pos_dist(generator_);
            return {x, static_cast<double>(start.y) + 0.4};
        } else {
            int min_y = std::min(start.y, end.y);
            int max_y = std::max(start.y, end.y);
            std::uniform_real_distribution<double> pos_dist(min_y + 0.4, max_y - 0.4);
            double y = pos_dist(generator_);
            return {static_cast<double>(start.x) + 0.4, y};
        }
    } else {
        const auto& first_road = roads[0];
        const auto& start = first_road.GetStart();
        return {static_cast<double>(start.x), static_cast<double>(start.y)};
    }
}

    void GameSessionManager::UpdateGameState(int time_delta_ms)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &[map_id, session] : sessions_)
        {
            auto map = game_.FindMap(map_id);
            if (!map)
                continue;

            auto roads_graph = BuildRoadsGraph(map);

            for (auto &[player_id, player] : session->GetPlayers())
            {
                if (player->GetSpeedX() == 0.0 && player->GetSpeedY() == 0.0)
                {
                    continue;
                }

                double current_x = player->GetPosX();
                double current_y = player->GetPosY();
                double speed_x = player->GetSpeedX();
                double speed_y = player->GetSpeedY();

                auto [should_stop, new_pos] = CalculateMove(current_x, current_y, speed_x, speed_y,
                                                            time_delta_ms, roads_graph);

                player->SetPosition(new_pos.first, new_pos.second);

                if (should_stop)
                {
                    player->SetSpeed(0.0, 0.0);
                }
            }
        }
    }

    std::unordered_map<Point, std::vector<const Road *>, model::PointHash>
    GameSessionManager::BuildRoadsGraph(const Map *map)
    {
        std::unordered_map<Point, std::vector<const Road *>, model::PointHash> roads_graph;

        for (const auto &road : map->GetRoads())
        {
            auto start = road.GetStart();
            auto end = road.GetEnd();

            if (road.IsHorizontal())
            {
                int start_x, end_x;
                if (start.x < end.x)
                {
                    start_x = start.x;
                    end_x = end.x;
                }
                else
                {
                    start_x = end.x;
                    end_x = start.x;
                }

                for (; start_x <= end_x; ++start_x)
                {
                    Point p = {start_x, start.y};
                    roads_graph[p].push_back(&road);
                }
            }
            else
            {
                int start_y, end_y;
                if (start.y < end.y)
                {
                    start_y = start.y;
                    end_y = end.y;
                }
                else
                {
                    start_y = end.y;
                    end_y = start.y;
                }

                for (; start_y <= end_y; ++start_y)
                {
                    Point p = {start.x, start_y};
                    roads_graph[p].push_back(&road);
                }
            }
        }

        return roads_graph;
    }

    bool GameSessionManager::InBounds(const Road *road, double x, double y) const
    {
        auto start_point = road->GetStart();
        auto end_point = road->GetEnd();
        double max_x, min_x, max_y, min_y;

        if (road->IsHorizontal())
        {
            if (start_point.x < end_point.x)
            {
                min_x = static_cast<double>(start_point.x) - 0.4;
                max_x = static_cast<double>(end_point.x) + 0.4;
            }
            else
            {
                min_x = static_cast<double>(end_point.x) - 0.4;
                max_x = static_cast<double>(start_point.x) + 0.4;
            }
            min_y = static_cast<double>(start_point.y) - 0.4;
            max_y = static_cast<double>(start_point.y) + 0.4;
        }
        else
        {
            if (start_point.y < end_point.y)
            {
                min_y = static_cast<double>(start_point.y) - 0.4;
                max_y = static_cast<double>(end_point.y) + 0.4;
            }
            else
            {
                min_y = static_cast<double>(end_point.y) - 0.4;
                max_y = static_cast<double>(start_point.y) + 0.4;
            }
            min_x = static_cast<double>(start_point.x) - 0.4;
            max_x = static_cast<double>(start_point.x) + 0.4;
        }

        bool result = (x <= max_x && x >= min_x && y <= max_y && y >= min_y);

        return result;
    }

    double GameSessionManager::GetMaxPossible(const Road *road, const std::string &direction) const
    {
        double result = 0.0;

        if (direction == "NORTH")
        {
            result = static_cast<double>(std::min(road->GetEnd().y, road->GetStart().y)) - 0.4;
        }
        else if (direction == "SOUTH")
        {
            result = static_cast<double>(std::max(road->GetEnd().y, road->GetStart().y)) + 0.4;
        }
        else if (direction == "EAST")
        {
            result = static_cast<double>(std::max(road->GetEnd().x, road->GetStart().x)) + 0.4;
        }
        else if (direction == "WEST")
        {
            result = static_cast<double>(std::min(road->GetEnd().x, road->GetStart().x)) - 0.4;
        }

        return result;
    }

    std::pair<bool, std::pair<double, double>> GameSessionManager::CalculateMove(
        double x, double y, double speed_x, double speed_y, int delta,
        const std::unordered_map<Point, std::vector<const Road *>, model::PointHash> &roads_graph) const
    {

        double in_seconds = static_cast<double>(delta) / 1000.0;
        double end_x = x + speed_x * in_seconds;
        double end_y = y + speed_y * in_seconds;

        Point rounded = {static_cast<int>(std::round(x)), static_cast<int>(std::round(y))};
        
        std::vector<Point> points_to_check = {rounded};

        const double EPSILON = 0.1;
        if (std::abs(x - std::round(x)) < EPSILON)
        {
            points_to_check.push_back({rounded.x - 1, rounded.y});
            points_to_check.push_back({rounded.x + 1, rounded.y});
        }
        if (std::abs(y - std::round(y)) < EPSILON)
        {
            points_to_check.push_back({rounded.x, rounded.y - 1});
            points_to_check.push_back({rounded.x, rounded.y + 1});
        }

        for (const auto &point : points_to_check)
        {
            auto it = roads_graph.find(point);
            if (it != roads_graph.end())
            {
                for (const auto &road : it->second)
                {
                    bool in_bounds = InBounds(road, end_x, end_y);

                    if (in_bounds)
                    {
                        return {false, {end_x, end_y}};
                    }
                }

                for (const auto &road : it->second)
                {
                    if (road->IsHorizontal() && speed_y == 0.0)
                    {
                        if (speed_x > 0)
                        {
                            double max_x = GetMaxPossible(road, "EAST");
                            return {true, {max_x, y}};
                        }
                        else
                        {
                            double max_x = GetMaxPossible(road, "WEST");
                            return {true, {max_x, y}};
                        }
                    }

                    if (road->IsVertical() && speed_x == 0.0)
                    {
                        if (speed_y > 0)
                        {
                            double max_y = GetMaxPossible(road, "SOUTH");
                            return {true, {x, max_y}};
                        }
                        else
                        {
                            double max_y = GetMaxPossible(road, "NORTH");
                            return {true, {x, max_y}};
                        }
                    }
                }
            }
        }

        if (speed_x == 0.0)
        {
            end_y = speed_y > 0 ? std::round(y) + 0.4 : std::round(y) - 0.4;
            return {true, {x, end_y}};
        }

        end_x = speed_x > 0 ? std::round(x) + 0.4 : std::round(x) - 0.4;
        return {true, {end_x, y}};
    }

    std::shared_ptr<GameSession> GameSessionManager::FindSessionUnsafe(const Map::Id &map_id)
    {
        auto it = sessions_.find(map_id);
        return it != sessions_.end() ? it->second : nullptr;
    }

    std::shared_ptr<GameSession> GameSessionManager::CreateSessionUnsafe(const Map::Id &map_id)
    {
        auto session = std::make_shared<GameSession>(map_id);
        sessions_[map_id] = session;
        return session;
    }

    std::shared_ptr<Player> GameSessionManager::FindPlayerByTokenUnsafe(const std::string &auth_token)
    {
        auto it = players_by_token_.find(auth_token);
        return it != players_by_token_.end() ? it->second : nullptr;
    }

} // namespace model