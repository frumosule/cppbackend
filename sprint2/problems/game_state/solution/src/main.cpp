#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
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

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-files-path>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // Настройка логирования
        setup_logging();
        
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
        
        // 2. Создаем менеджер игровых сессий
        model::GameSessionManager session_manager(game);
        
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

        // 5. Создаём обработчик HTTP-запросов и связываем его с моделью игры и менеджером сессий
        http_handler::RequestHandler base_handler{game, session_manager, argv[2]};
        
        // Оборачиваем в логирующий декоратор
        LoggingRequestHandler<http_handler::RequestHandler> handler{base_handler};

        // 6. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        
        // Логируем запуск сервера
        ServerLogger::log_server_start(port, address.to_string());
        
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // 7. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        
        // Логируем успешное завершение
        ServerLogger::log_server_exit(0);
        
    } catch (const std::exception& ex) {
        // Логируем аварийное завершение
        ServerLogger::log_server_exit(EXIT_FAILURE, ex.what());
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}