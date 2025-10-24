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
        
        std::string client_ip = "unknown";
        if (req.version() >= 11) {
            //доделать потом
        }
        
        ServerLogger::log_request(client_ip, std::string(req.target()), std::string(req.method_string()));
        
        handler_(std::move(req), [&, start_time, client_ip](auto&& response) {
            auto end_time = std::chrono::steady_clock::now();
            auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();
            
            std::string content_type;
            auto content_type_it = response.find(http::field::content_type);
            if (content_type_it != response.end()) {
                content_type = std::string(content_type_it->value());
            }
            
            ServerLogger::log_response(client_ip, response_time, response.result_int(), content_type);
            
            send(std::forward<decltype(response)>(response));
        });
    }

private:
    RequestHandler& handler_;
};