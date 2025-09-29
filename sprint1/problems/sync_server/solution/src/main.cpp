#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

// Функция для создания строки ответа
std::string MakeResponseBody(const std::string& target) {
    return "Hello, " + target;
}

// Функция для обработки HTTP-запросов
http::response<http::string_body> HandleRequest(http::request<http::string_body>&& req) {
    // Проверяем метод запроса
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        // Метод не GET и не HEAD - возвращаем 405
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "text/html");
        res.set(http::field::allow, "GET, HEAD");
        res.body() = "Invalid method";
        res.prepare_payload();
        return res;
    }

    // Извлекаем target из URL (убираем ведущий '/')
    std::string target = std::string(req.target());
    if (!target.empty() && target[0] == '/') {
        target = target.substr(1);
    }
    
    // Создаем тело ответа
    std::string body = MakeResponseBody(target);
    
    // Создаем ответ
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/html");
    res.set(http::field::content_length, std::to_string(body.length()));
    
    // Для GET запроса добавляем тело, для HEAD - нет
    if (req.method() == http::verb::get) {
        res.body() = body;
    }
    
    return res;
}

int main() {
    try {
        const auto address = net::ip::make_address("0.0.0.0");
        const unsigned short port = 8080;

        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};
        
        std::cout << "Server has started..." << std::endl;

        while (true) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);

            try {
                beast::flat_buffer buffer;
                http::request<http::string_body> req;
                http::read(socket, buffer, req);

                auto res = HandleRequest(std::move(req));
                http::write(socket, res);
            }
            catch (const std::exception& e) {
                std::cerr << "Error handling request: " << e.what() << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}