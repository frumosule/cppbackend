#include "logger.h"
#include "formatter.h"
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

void ServerLogger::log_server_start(unsigned short port, const std::string& address) {
    boost::json::value data = {
        {"port", port},
        {"address", address}
    };
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "server started";
}

void ServerLogger::log_server_exit(int exit_code, const std::string& exception_what) {
    boost::json::value data = {{"code", exit_code}};
    
    if (!exception_what.empty()) {
        data.as_object()["exception"] = exception_what;
    }
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "server exited";
}

void ServerLogger::log_error(const beast::error_code& ec, const std::string& where) {
    boost::json::value data = {
        {"code", ec.value()},
        {"text", ec.message()},
        {"where", where}
    };
    
    BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, data)
                            << "error";
}

void ServerLogger::log_request(const std::string& ip, const std::string& uri, const std::string& method) {
    boost::json::value data = {
        {"ip", ip},
        {"URI", uri},
        {"method", method}
    };
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "request received";
}

void ServerLogger::log_response(const std::string& ip, long response_time, int code, const std::string& content_type) {
    boost::json::value data = {
        {"ip", ip},
        {"response_time", response_time},
        {"code", code},
        {"content_type", content_type.empty() ? "null" : content_type}
    };
    
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data)
                           << "response sent";
}