#pragma once

#include <string>
#include <boost/beast.hpp>
#include <boost/json.hpp>

namespace beast = boost::beast;

class ServerLogger {
public:
    static void log_server_start(unsigned short port, const std::string& address = "0.0.0.0");
    static void log_server_exit(int exit_code = 0, const std::string& exception_what = "");
    static void log_error(const beast::error_code& ec, const std::string& where);
    
    // Для логирования запросов и ответов
    static void log_request(const std::string& ip, const std::string& uri, const std::string& method);
    static void log_response(const std::string& ip, long response_time, int code, const std::string& content_type);
};