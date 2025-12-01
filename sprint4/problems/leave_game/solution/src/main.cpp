#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <fstream>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"
#include "logging/formatter.h"
#include "logging/logger.h"
#include "logging/logging_handler.h"
#include "game_session_manager.h"
#include "ticker.h"
#include "application_listener.h"
#include "serializing_listener.h"
#include "retired_players_manager.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace po = boost::program_options;

namespace
{

    template <typename Fn>
    void RunWorkers(unsigned n, const Fn &fn)
    {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        while (--n)
        {
            workers.emplace_back(fn);
        }
        fn();
    }

    struct Args
    {
        int tick_period = 0;
        std::string config_file;
        std::string www_root;
        bool randomize_spawn_points = false;
        std::string state_file;
        int save_state_period = 0;
    };

    [[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char *const argv[])
    {
        po::options_description desc{"Allowed options"s};

        Args args;
        desc.add_options()("help,h", "produce help message")("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")("config-file,c", po::value(&args.config_file)->required()->value_name("file"s), "set config file path")("www-root,w", po::value(&args.www_root)->required()->value_name("dir"s), "set static files root")("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions")("state-file", po::value(&args.state_file)->value_name("path"s), "path to state file")("save-state-period", po::value(&args.save_state_period)->value_name("milliseconds"s), "auto-save period");

        po::variables_map vm;
        try
        {
            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.contains("help"s))
            {
                std::cout << desc << std::endl;
                return std::nullopt;
            }

            po::notify(vm);
        }
        catch (const po::error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            std::cout << desc << std::endl;
            return std::nullopt;
        }

        return args;
    }

} // namespace

int main(int argc, const char *argv[])
{
    try
    {
        auto args_opt = ParseCommandLine(argc, argv);
        if (!args_opt)
        {
            return EXIT_SUCCESS;
        }

        auto args = *args_opt;

        setup_logging();

        // Загружаем конфигурацию и получаем JSON объект
        auto load_result = json_loader::LoadGameWithLootConfig(args.config_file);
        model::Game game = std::move(load_result.game);

        auto config_content = json_loader::detail::LoadFileContent(args.config_file);
        auto config = json_loader::detail::ParseJson(config_content);

        model::GameSessionManager session_manager(game);

        if (!args.state_file.empty() && std::filesystem::exists(args.state_file))
        {
            try
            {
                std::ifstream state_file(args.state_file, std::ios::binary);
                if (state_file)
                {
                    session_manager.Deserialize(state_file);
                    std::cout << "Game state restored from: " << args.state_file << std::endl;
                }
            }
            catch (const std::exception &ex)
            {
                std::cerr << "Failed to restore state from " << args.state_file << ": " << ex.what() << std::endl;
                return EXIT_FAILURE;
            }
        }

        for (const auto &[map_id, loot_values] : load_result.map_loot_values)
        {
            session_manager.SetLootValuesForMap(map_id, loot_values);
            std::cout << "Set loot values for map " << *map_id << " with "
                      << loot_values.size() << " item types" << std::endl;
        }

        if (load_result.loot_config)
        {
            auto &loot_config = load_result.loot_config.value();
            session_manager.InitializeLootGenerator(loot_config.period, loot_config.probability);
            std::cout << "Loot generator initialized: period=" << loot_config.period
                      << "s, probability=" << loot_config.probability << std::endl;
        }
        else
        {
            std::cout << "Loot generator: no configuration found" << std::endl;
        }

        session_manager.SetRandomSpawn(args.randomize_spawn_points);

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number)
                           {
            if (!ec) {
                ioc.stop();
            } });

        // ------------------------------------------------------------
        // ИНИЦИАЛИЗАЦИЯ БАЗЫ ДАННЫХ (исправленная часть)
        // ------------------------------------------------------------
        const char *db_url = std::getenv("GAME_DB_URL");
        std::shared_ptr<database::ConnectionPool> db_pool;
        std::shared_ptr<database::StatProvider> stat_provider;

        if (db_url)
        {
            std::cout << "GAME_DB_URL found: " << db_url << std::endl;

            try
            {
                // Создаем пул соединений
                db_pool = std::make_shared<database::ConnectionPool>(
                    std::max(1u, num_threads),
                    [db_url]()
                    {
                        std::cout << "Creating database connection..." << std::endl;
                        return std::make_shared<pqxx::connection>(db_url);
                    });

                // Инициализируем БД (создаем таблицы если их нет)
                database::InitializeDB(db_pool);
                std::cout << "Database initialized successfully" << std::endl;

                // Создаем StatProvider
                stat_provider = std::make_shared<database::StatProvider>(db_pool);
                std::cout << "StatProvider created" << std::endl;

                // Настройка времени бездействия из конфига
                double retirement_time_seconds = 60.0; // по умолчанию 1 минута
                if (config.is_object() && config.as_object().contains("dogRetirementTime"))
                {
                    retirement_time_seconds = config.at("dogRetirementTime").as_double();
                    std::cout << "Retirement time from config: " << retirement_time_seconds << " seconds" << std::endl;
                }

                auto retirement_timeout = std::chrono::milliseconds(
                    static_cast<int>(retirement_time_seconds * 1000));

                session_manager.SetRetirementTimeout(retirement_timeout);
                session_manager.SetStatProvider(stat_provider);
                std::cout << "Retirement system configured" << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << "ERROR initializing database: " << e.what() << std::endl;
                std::cerr << "Retirement system will be disabled" << std::endl;
                // Продолжаем без БД
                stat_provider = nullptr;
            }
        }
        else
        {
            std::cout << "GAME_DB_URL not set - retirement system disabled" << std::endl;
            stat_provider = nullptr;
        }

        // Создаем RequestHandler и передаем stat_provider
        http_handler::RequestHandler base_handler{
            game,
            session_manager,
            args.www_root,
            std::move(load_result.map_loot_types),
            stat_provider // передаем StatProvider (может быть nullptr)
        };

        std::cout << "RequestHandler created" << std::endl;

        if (!args.state_file.empty())
        {
            auto save_period = args.save_state_period > 0
                                   ? std::chrono::milliseconds(args.save_state_period)
                                   : std::chrono::milliseconds(0);

            std::cout << "State file: " << args.state_file
                      << ", save period: " << save_period.count() << "ms" << std::endl;

            auto serializing_listener = std::make_shared<infrastructure::SerializingListener>(
                session_manager, args.state_file, save_period);
            session_manager.AddListener(serializing_listener);
        }

        std::shared_ptr<Ticker> ticker;
        if (args.tick_period > 0)
        {
            base_handler.SetAutoTickMode(true);
            std::cout << "Auto-tick mode enabled with period: " << args.tick_period << "ms" << std::endl;

            auto period = std::chrono::milliseconds(args.tick_period);

            ticker = std::make_shared<Ticker>(ioc, period,
                                              [&session_manager](std::chrono::milliseconds delta)
                                              {
                                                  session_manager.UpdateGameState(delta.count());
                                              });
            ticker->Start();
        }
        else
        {
            std::cout << "Manual tick mode (use POST /api/v1/game/tick)" << std::endl;
            base_handler.SetAutoTickMode(false);
        }

        // Таймер для проверки неактивных игроков
        std::shared_ptr<Ticker> retirement_ticker;
        if (db_pool && stat_provider)
        {
            std::cout << "Starting retirement ticker..." << std::endl;
            retirement_ticker = std::make_shared<Ticker>(ioc, std::chrono::seconds(1),
                                                         [&session_manager]([[maybe_unused]] std::chrono::milliseconds delta)
                                                         {
                                                             try
                                                             {
                                                                 session_manager.CheckInactivePlayers();
                                                             }
                                                             catch (const std::exception &e)
                                                             {
                                                                 std::cerr << "Error in CheckInactivePlayers: " << e.what() << std::endl;
                                                             }
                                                         });
            retirement_ticker->Start();
            std::cout << "Retirement ticker started" << std::endl;
        }
        else
        {
            std::cout << "Retirement ticker NOT started (no db pool or stat provider)" << std::endl;
        }

        if (args.randomize_spawn_points)
        {
            std::cout << "Random spawn points enabled" << std::endl;
        }

        std::cout << "Loaded " << game.GetMaps().size() << " maps" << std::endl;
        for (const auto &map : game.GetMaps())
        {
            std::cout << "  Map: " << *map.GetId() << " (" << map.GetName()
                      << "), loot types: " << map.GetLootTypesCount() << std::endl;

            auto loot_values = session_manager.GetLootValuesForMap(map.GetId());
            if (!loot_values.empty())
            {
                std::cout << "    Loot values: ";
                for (const auto &[type, value] : loot_values)
                {
                    std::cout << "type" << type << "=" << value << " ";
                }
                std::cout << std::endl;
            }
        }

        LoggingRequestHandler<http_handler::RequestHandler> handler{base_handler};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        ServerLogger::LogServerStart(port, address.to_string());

        http_server::ServeHttp(ioc, {address, port}, [&handler](auto &&req, auto &&send)
                               { handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send)); });

        RunWorkers(std::max(1u, num_threads), [&ioc]
                   { ioc.run(); });

        if (!args.state_file.empty())
        {
            session_manager.NotifyShutdown();
        }

        // Останавливаем таймеры в правильном порядке
        if (retirement_ticker)
        {
            retirement_ticker->Stop();
        }

        if (ticker)
        {
            ticker->Stop();
        }

        ServerLogger::LogServerExit(0);
    }
    catch (const std::exception &ex)
    {
        ServerLogger::LogServerExit(EXIT_FAILURE, ex.what());
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}