#include "logger.h"
#include "formatter.h"
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <iostream>
#include <string>  
#include <cstdint>
using namespace std::string_literals;

void ServerLogger::LogServerStart(unsigned short port, const std::string& address) {
    boost::json::value data = {
        {"port", port},
        {"address", address}
    };
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "Server has started";
}

void ServerLogger::LogServerExit(int exit_code, const std::string& exception_what) {
    boost::json::value data = {{"code", exit_code}};
    
    if (!exception_what.empty()) {
        data.as_object()["exception"] = exception_what;
    }
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "server exited";
}

void ServerLogger::LogError(const beast::error_code& ec, const std::string& where) {
    boost::json::value data = {
        {"code", ec.value()},
        {"text", ec.message()},
        {"where", where}
    };
    
    BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, data)
                            << "error";
}

void ServerLogger::LogRequest(const std::string& ip, const std::string& uri, const std::string& method) {
    boost::json::value data = {
        {"ip", ip},
        {"URI", uri},
        {"method", method}
    };
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "request received";
}

void ServerLogger::LogResponse(const std::string& ip, int64_t response_time, int32_t code, const std::string& content_type) {
    boost::json::value data = {
        {"ip", ip},
        {"response_time", response_time},
        {"code", code},
        {"content_type", content_type.empty() ? "null" : content_type}
    };
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "response sent";
}