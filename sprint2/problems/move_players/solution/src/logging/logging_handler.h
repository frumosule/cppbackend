#pragma once

#include <boost/beast.hpp>
#include <chrono>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;

template<typename RequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(RequestHandler& handler) : handler_(handler) {}
    
    template <typename Send>
    void operator()(http::request<http::string_body>&& req, Send&& send) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Получаем IP-адрес клиента
        std::string client_ip = "unknown";
        if (req.version() >= 11) {
            // Можно добавить логику для получения реального IP
        }
        
        // Логируем запрос
        ServerLogger::log_request(client_ip, std::string(req.target()), std::string(req.method_string()));
        
        // Вызываем оригинальный обработчик
        handler_(std::move(req), [&, start_time, client_ip](auto&& response) {
            auto end_time = std::chrono::steady_clock::now();
            auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();
            
            // Получаем content_type
            std::string content_type;
            auto content_type_it = response.find(http::field::content_type);
            if (content_type_it != response.end()) {
                content_type = std::string(content_type_it->value());
            }
            
            // Логируем ответ
            ServerLogger::log_response(client_ip, response_time, response.result_int(), content_type);
            
            // Передаем ответ дальше
            send(std::forward<decltype(response)>(response));
        });
    }

private:
    RequestHandler& handler_;
};