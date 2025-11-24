#pragma once

#include <string>
#include <cstdint>
#include <boost/beast.hpp>
#include <boost/json.hpp>

namespace beast = boost::beast;

class ServerLogger {
public:
    static void LogServerStart(unsigned short port, const std::string& address = "0.0.0.0");
    static void LogServerExit(int exit_code = 0, const std::string& exception_what = "");
    static void LogError(const beast::error_code& ec, const std::string& where);
    
    // Для логирования запросов и ответов
    static void LogRequest(const std::string& ip, const std::string& uri, const std::string& method);
    static void LogResponse(const std::string& ip, int64_t response_time, int32_t code, const std::string& content_type);
};