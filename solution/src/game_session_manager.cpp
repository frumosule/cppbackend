#include "game_session_manager.h"
#include "collision_handler.h"
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
        auto session_data = FindSessionUnsafe(map_id);
        if (!session_data)
        {
            return nullptr;
        }

        auto game_session = std::make_shared<GameSession>(map_id);
        for (const auto &[player_id, player] : session_data->players)
        {
            game_session->AddPlayer(player);
        }
        return game_session;
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

        size_t bag_capacity = GetBagCapacityForMap(map_id);

        auto player = std::make_shared<Player>(next_player_id_++, player_name, auth_token,
                                               map_id, pos_x, pos_y, bag_capacity);

        session->players[player->GetId()] = player;
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

        return session->players;
    }

    std::pair<double, double> GameSessionManager::GetRandomPositionOnRoad(const Map::Id &map_id)
    {
        auto map = game_.FindMap(map_id);
        if (!map)
        {
            throw std::runtime_error("Map not found for getting random position");
        }

        const auto &roads = map->GetRoads();
        if (roads.empty())
        {
            throw std::runtime_error("No roads on the map");
        }

        if (randomize_spawn_)
        {
            std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
            const auto &road = roads[road_dist(generator_)];

            const auto &start = road.GetStart();
            const auto &end = road.GetEnd();
            static constexpr double ROAD_OFFSET = 0.4;

            if (road.IsHorizontal())
            {
                int min_x = std::min(start.x, end.x);
                int max_x = std::max(start.x, end.x);
                std::uniform_real_distribution<double> pos_dist(min_x + ROAD_OFFSET, max_x - ROAD_OFFSET);
                double x = pos_dist(generator_);
                return {x, static_cast<double>(start.y) + ROAD_OFFSET};
            }
            else
            {
                int min_y = std::min(start.y, end.y);
                int max_y = std::max(start.y, end.y);
                std::uniform_real_distribution<double> pos_dist(min_y + ROAD_OFFSET, max_y - ROAD_OFFSET);
                double y = pos_dist(generator_);
                return {static_cast<double>(start.x) + ROAD_OFFSET, y};
            }
        }
        else
        {
            const auto &first_road = roads[0];
            const auto &start = first_road.GetStart();
            return {static_cast<double>(start.x), static_cast<double>(start.y)};
        }
    }

    void GameSessionManager::GenerateLootForSession(SessionData &session, const Map::Id &map_id, int time_delta_ms)
    {
        auto map = game_.FindMap(map_id);
        if (!map || !loot_generator_ || map->GetLootTypesCount() == 0)
        {
            return;
        }

        auto loot_values = GetLootValuesForMap(map_id);

        unsigned current_loot_count = session.lost_objects.size();
        unsigned looter_count = session.players.size();

        unsigned new_loot_count = loot_generator_->Generate(
            std::chrono::milliseconds(time_delta_ms),
            current_loot_count,
            looter_count);

        for (unsigned i = 0; i < new_loot_count; ++i)
        {
            LostObject new_obj(
                session.next_lost_object_id++,
                0,
                Point{0, 0},
                0);

            std::uniform_int_distribution<int> type_dist(0, map->GetLootTypesCount() - 1);
            int item_type = type_dist(generator_);
            new_obj.type = item_type;

            auto value_it = loot_values.find(item_type);
            if (value_it != loot_values.end())
            {
                new_obj.value = value_it->second;
            }
            else
            {
                new_obj.value = 0;
            }

            Point pos = map->GetRandomPoint(generator_);
            new_obj.position = pos;

            session.lost_objects[new_obj.id] = new_obj;

            std::cout << "Generated loot: id=" << new_obj.id
                      << ", type=" << new_obj.type
                      << ", value=" << new_obj.value
                      << ", pos=(" << pos.x << "," << pos.y << ")" << std::endl;
        }
    }

    void GameSessionManager::SetLootValuesForMap(const Map::Id &map_id, std::unordered_map<int, int> loot_values)
    {
        map_loot_values_[map_id] = std::move(loot_values);
    }

    std::unordered_map<int, int> GameSessionManager::GetLootValuesForMap(const Map::Id &map_id) const
    {
        auto it = map_loot_values_.find(map_id);
        if (it != map_loot_values_.end())
        {
            return it->second;
        }
        return {};
    }

    const std::unordered_map<LostObject::Id, LostObject> &GameSessionManager::GetLostObjects(const Map::Id &map_id) const
    {
        static std::unordered_map<LostObject::Id, LostObject> empty;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(map_id);
        if (it != sessions_.end())
        {
            return it->second->lost_objects;
        }
        return empty;
    }

    void GameSessionManager::InitializeLootGenerator(double base_interval, double probability)
    {
        auto base_interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double>(base_interval));

        loot_generator_ = std::make_unique<loot_gen::LootGenerator>(
            base_interval_ms,
            probability,
            [this]()
            {
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                return dist(generator_);
            });
    }

    void GameSessionManager::UpdateGameState(int time_delta_ms)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &[map_id, session] : sessions_)
        {
            auto map = game_.FindMap(map_id);
            if (!map)
                continue;

            GenerateLootForSession(*session, map_id, time_delta_ms);

            double time_delta_seconds = static_cast<double>(time_delta_ms) / 1000.0;
            auto collision_result = CollisionHandler::ProcessCollisions(
                session->players,
                session->lost_objects,
                map->GetOffices(),
                time_delta_seconds);

            for (const auto &[player_id, collected_items] : collision_result.collected_items)
            {
                auto player = session->players[player_id];
                for (LostObject::Id item_id : collected_items)
                {
                    auto it = session->lost_objects.find(item_id);
                    if (it != session->lost_objects.end() && player->AddToBag(item_id, it->second.type, it->second.value))
                    {
                        session->lost_objects.erase(it);
                        std::cout << "Player " << player_id << " collected item " << item_id
                                  << " worth " << it->second.value << " points" << std::endl;
                    }
                }
            }

            for (const auto &[player_id, returned] : collision_result.returned_to_office)
            {
                if (returned)
                {
                    auto player = session->players[player_id];

                    int total_score = 0;
                    for (const auto &bag_item : player->GetBag())
                    {
                        total_score += bag_item.value;
                    }

                    if (total_score > 0)
                    {
                        player->AddScore(total_score);
                        std::cout << "Player " << player_id << " returned items to office and earned "
                                  << total_score << " points" << std::endl;
                    }

                    player->ClearBag();
                }
            }

            auto roads_graph = BuildRoadsGraph(map);

            for (auto &[player_id, player] : session->players)
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

        std::cout << "=== Building Roads Graph ===" << std::endl;
        std::cout << "=== Building Roads Graph for map: " << *map->GetId() << " ===" << std::endl;
        std::cout << "Map has " << map->GetRoads().size() << " roads" << std::endl;

        for (const auto &road : map->GetRoads())
        {
            auto start = road.GetStart();
            auto end = road.GetEnd();

            std::cout << "Road: (" << start.x << "," << start.y
                      << ") to (" << end.x << "," << end.y
                      << "), " << (road.IsHorizontal() ? "HORIZONTAL" : "VERTICAL") << std::endl;

            std::cout << "Processing road: (" << start.x << "," << start.y
                      << ") to (" << end.x << "," << end.y
                      << "), Horizontal: " << road.IsHorizontal() << std::endl;

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

                std::cout << "  Adding horizontal points from " << start_x << " to " << end_x << " at y=" << start.y << std::endl;

                for (; start_x <= end_x; ++start_x)
                {
                    Point p = {start_x, start.y};
                    roads_graph[p].push_back(&road);
                    std::cout << "    Point (" << p.x << "," << p.y << ")" << std::endl;
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

                std::cout << "  Adding vertical points from " << start_y << " to " << end_y << " at x=" << start.x << std::endl;

                for (; start_y <= end_y; ++start_y)
                {
                    Point p = {start.x, start_y};
                    roads_graph[p].push_back(&road);
                    std::cout << "    Point (" << p.x << "," << p.y << ")" << std::endl;
                }
            }
        }

        std::cout << "=== Roads Graph Built ===" << std::endl;
        std::cout << "Total points in graph: " << roads_graph.size() << std::endl;

        auto check_point = roads_graph.find({1, 1});
        if (check_point != roads_graph.end())
        {
            std::cout << "Point (1,1) FOUND in graph with " << check_point->second.size() << " roads" << std::endl;
        }
        else
        {
            std::cout << "Point (1,1) NOT FOUND in graph!" << std::endl;
        }

        return roads_graph;
    }

    bool GameSessionManager::InBounds(const Road *road, double x, double y) const
    {
        auto start_point = road->GetStart();
        auto end_point = road->GetEnd();
        double max_x, min_x, max_y, min_y;
        static constexpr double ROAD_OFFSET = 0.4;

        if (road->IsHorizontal())
        {
            if (start_point.x < end_point.x)
            {
                min_x = static_cast<double>(start_point.x) - ROAD_OFFSET;
                max_x = static_cast<double>(end_point.x) + ROAD_OFFSET;
            }
            else
            {
                min_x = static_cast<double>(end_point.x) - ROAD_OFFSET;
                max_x = static_cast<double>(start_point.x) + ROAD_OFFSET;
            }
            min_y = static_cast<double>(start_point.y) - ROAD_OFFSET;
            max_y = static_cast<double>(start_point.y) + ROAD_OFFSET;
        }
        else
        {
            if (start_point.y < end_point.y)
            {
                min_y = static_cast<double>(start_point.y) - ROAD_OFFSET;
                max_y = static_cast<double>(end_point.y) + ROAD_OFFSET;
            }
            else
            {
                min_y = static_cast<double>(end_point.y) - ROAD_OFFSET;
                max_y = static_cast<double>(start_point.y) + ROAD_OFFSET;
            }
            min_x = static_cast<double>(start_point.x) - ROAD_OFFSET;
            max_x = static_cast<double>(start_point.x) + ROAD_OFFSET;
        }

        bool result = (x <= max_x && x >= min_x && y <= max_y && y >= min_y);

        std::cout << "InBounds: road(" << start_point.x << "," << start_point.y
                  << ")-(" << end_point.x << "," << end_point.y
                  << "), point(" << x << "," << y
                  << "), bounds[" << min_x << "," << max_x << "][" << min_y << "," << max_y
                  << "] = " << (result ? "IN" : "OUT") << std::endl;

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

        std::cout << "GetMaxPossible: direction=" << direction << ", result=" << result << std::endl;
        return result;
    }

    std::pair<bool, std::pair<double, double>> GameSessionManager::CalculateMove(
        double x, double y, double speed_x, double speed_y, int delta,
        const std::unordered_map<Point, std::vector<const Road *>, model::PointHash> &roads_graph) const
    {

        static constexpr int MILLISECONDS_IN_SECOND = 1000;
        double in_seconds = static_cast<double>(delta) / MILLISECONDS_IN_SECOND;
        double end_x = x + speed_x * in_seconds;
        double end_y = y + speed_y * in_seconds;
        static constexpr double POSITION_EPSILON = 0.001;
        static constexpr double POSITION_Y = 0.4;
        static constexpr double POSITION_X = 28.587;

        if (std::abs(x - POSITION_X) < POSITION_EPSILON && std::abs(y - POSITION_Y) < POSITION_EPSILON)
        {
            std::cout << "=== SPECIAL DEBUG for test position ===" << std::endl;
            std::cout << "Moving from test position: (" << x << ", " << y << ")" << std::endl;
            std::cout << "Target: (" << end_x << ", " << end_y << ")" << std::endl;
        }

        std::cout << "=== CalculateMove ===" << std::endl;
        std::cout << "Start: (" << x << ", " << y << ")" << std::endl;
        std::cout << "Speed: (" << speed_x << ", " << speed_y << ")" << std::endl;
        std::cout << "Target: (" << end_x << ", " << end_y << ")" << std::endl;

        Point rounded = {static_cast<int>(std::round(x)), static_cast<int>(std::round(y))};
        std::cout << "Rounded point: (" << rounded.x << ", " << rounded.y << ")" << std::endl;

        std::vector<Point> points_to_check = {rounded};

        const double EPSILON = std::numeric_limits<double>::epsilon();
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
                std::cout << "Found " << it->second.size() << " roads at point (" << point.x << "," << point.y << ")" << std::endl;

                for (const auto &road : it->second)
                {
                    bool in_bounds = InBounds(road, end_x, end_y);
                    std::cout << "Road from (" << road->GetStart().x << "," << road->GetStart().y
                              << ") to (" << road->GetEnd().x << "," << road->GetEnd().y
                              << ") - InBounds: " << in_bounds << std::endl;

                    if (in_bounds)
                    {
                        std::cout << "MOVEMENT ALLOWED to target position" << std::endl;
                        return {false, {end_x, end_y}};
                    }
                }

                std::cout << "NO ROAD ALLOWS MOVEMENT TO TARGET - CHECKING BOUNDARIES" << std::endl;

                for (const auto &road : it->second)
                {
                    if (road->IsHorizontal() && speed_y == 0.0)
                    {
                        if (speed_x > 0)
                        {
                            double max_x = GetMaxPossible(road, "EAST");
                            std::cout << "Horizontal road - stopping at EAST boundary: " << max_x << std::endl;
                            return {true, {max_x, y}};
                        }
                        else
                        {
                            double max_x = GetMaxPossible(road, "WEST");
                            std::cout << "Horizontal road - stopping at WEST boundary: " << max_x << std::endl;
                            return {true, {max_x, y}};
                        }
                    }

                    if (road->IsVertical() && speed_x == 0.0)
                    {
                        if (speed_y > 0)
                        {
                            double max_y = GetMaxPossible(road, "SOUTH");
                            std::cout << "Vertical road - stopping at SOUTH boundary: " << max_y << std::endl;
                            return {true, {x, max_y}};
                        }
                        else
                        {
                            double max_y = GetMaxPossible(road, "NORTH");
                            std::cout << "Vertical road - stopping at NORTH boundary: " << max_y << std::endl;
                            return {true, {x, max_y}};
                        }
                    }
                }
            }
            else
            {
                std::cout << "NO ROADS FOUND at point (" << point.x << ", " << point.y << ")" << std::endl;
            }
        }

        std::cout << "NO ROADS FOUND IN ANY CHECKED POINTS - USING FALLBACK" << std::endl;

        if (speed_x == 0.0)
        {
            end_y = speed_y > 0 ? std::round(y) + 0.4 : std::round(y) - 0.4;
            std::cout << "Fallback Y movement: " << end_y << std::endl;
            return {true, {x, end_y}};
        }

        end_x = speed_x > 0 ? std::round(x) + 0.4 : std::round(x) - 0.4;
        std::cout << "Fallback X movement: " << end_x << std::endl;
        return {true, {end_x, y}};
    }

    std::shared_ptr<GameSessionManager::SessionData> GameSessionManager::FindSessionUnsafe(const Map::Id &map_id)
    {
        auto it = sessions_.find(map_id);
        return it != sessions_.end() ? it->second : nullptr;
    }

    std::shared_ptr<GameSessionManager::SessionData> GameSessionManager::CreateSessionUnsafe(const Map::Id &map_id)
    {
        auto session = std::make_shared<SessionData>();
        sessions_[map_id] = session;
        return session;
    }

    std::shared_ptr<Player> GameSessionManager::FindPlayerByTokenUnsafe(const std::string &auth_token)
    {
        auto it = players_by_token_.find(auth_token);
        return it != players_by_token_.end() ? it->second : nullptr;
    }

    size_t GameSessionManager::GetBagCapacityForMap(const Map::Id &map_id) const
    {
        auto map = game_.FindMap(map_id);
        if (!map)
        {
            return game_.GetDefaultBagCapacity();
        }

        if (map->HasBagCapacity())
        {
            return map->GetBagCapacity();
        }
        else
        {
            return game_.GetDefaultBagCapacity();
        }
    }

} // namespace model