#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"
#include "logging/formatter.h"
#include "logging/logger.h"
#include "logging/logging_handler.h"
#include "game_session_manager.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace po = boost::program_options;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

// Структура для хранения параметров командной строки
struct Args {
    int tick_period = 0; // 0 означает ручное управление тиками
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
};

// Функция для разбора командной строки
[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    po::options_description desc{"Allowed options"s};
    
    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file)->required()->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.www_root)->required()->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        
        if (vm.contains("help"s)) {
            std::cout << desc << std::endl;
            return std::nullopt;
        }
        
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << desc << std::endl;
        return std::nullopt;
    }

    return args;
}

// Класс для периодических тиков (добавьте перед main)
class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_{strand}
        , period_{period}
        , handler_{std::move(handler)} {
    }

    void Start() {
        last_tick_ = Clock::now();
        net::dispatch(strand_, [self = shared_from_this()] {
            self->ScheduleTick();
        });
    }

    void Stop() {
        net::dispatch(strand_, [self = shared_from_this()] {
            self->timer_.cancel();
        });
    }

private:
    void ScheduleTick() {
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
        });
    }

    void OnTick(sys::error_code ec) {
        using namespace std::chrono;
        
        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            
            try {
                handler_(delta);
            } catch (...) {
                // Игнорируем исключения в обработчике
            }
            ScheduleTick();
        }
    }

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        // Парсим командную строку
        auto args_opt = ParseCommandLine(argc, argv);
        if (!args_opt) {
            return EXIT_SUCCESS;
        }
        
        auto args = *args_opt;

        // Настройка логирования
        setup_logging();
        
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args.config_file);
        
        // 2. Создаем менеджер игровых сессий
        model::GameSessionManager session_manager(game);
        
        // Устанавливаем режим случайного спавна
        session_manager.SetRandomSpawn(args.randomize_spawn_points);
        
        // 3. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 4. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 5. Создаём обработчик HTTP-запросов
        http_handler::RequestHandler base_handler{game, session_manager, args.www_root};
        
        // Настраиваем режим авто-тиков
        std::shared_ptr<Ticker> ticker;
        if (args.tick_period > 0) {
            base_handler.SetAutoTickMode(true);
            std::cout << "Auto-tick mode enabled with period: " << args.tick_period << "ms" << std::endl;
            
            // Создаем strand для безопасного доступа к игровому состоянию
            auto game_strand = net::make_strand(ioc);
            
            // Запускаем автоматические тики
            auto period = std::chrono::milliseconds(args.tick_period);
            ticker = std::make_shared<Ticker>(game_strand, period,
                [&session_manager](std::chrono::milliseconds delta) {
                    session_manager.UpdateGameState(delta.count());
                });
            ticker->Start();
        } else {
            std::cout << "Manual tick mode (use POST /api/v1/game/tick)" << std::endl;
            base_handler.SetAutoTickMode(false);
        }
        
        if (args.randomize_spawn_points) {
            std::cout << "Random spawn points enabled" << std::endl;
        }

        // Оборачиваем в логирующий декоратор
        LoggingRequestHandler<http_handler::RequestHandler> handler{base_handler};

        // 6. Запустить обработчик HTTP-запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        
        ServerLogger::log_server_start(port, address.to_string());

        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // 7. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        
        // Останавливаем тикер при выходе
        if (ticker) {
            ticker->Stop();
        }
        
        ServerLogger::log_server_exit(0);
        
    } catch (const std::exception& ex) {
        ServerLogger::log_server_exit(EXIT_FAILURE, ex.what());
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}