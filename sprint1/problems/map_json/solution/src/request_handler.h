#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <sstream>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

using StringRequest = http_server::StringRequest;
using StringResponse = http_server::StringResponse;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game);

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    // Оставляем шаблонный operator() в заголовочном файле
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        StringRequest string_req;
        string_req.method(req.method());
        string_req.target(req.target());
        string_req.version(req.version());
        string_req.body() = req.body();
        string_req.prepare_payload();
        
        for (const auto& field : req) {
            string_req.set(field.name(), field.value());
        }
        
        auto response = HandleRequest(std::move(string_req));
        send(std::move(response));
    }

private:
    // Эти методы переносим в .cpp
    StringResponse HandleRequest(StringRequest&& req);
    StringResponse GetMapsList(const StringRequest& req);
    StringResponse GetMapById(const std::string& map_id, const StringRequest& req);

    template <typename T>
    StringResponse MakeJsonResponse(http::status status, T&& data, const StringRequest& req) {
        StringResponse response(status, req.version());
        response.set(http::field::content_type, "application/json");
        response.set(http::field::cache_control, "no-cache");
        
        std::stringstream ss;
        ss << std::forward<T>(data);
        response.body() = ss.str();
        
        response.prepare_payload();
        return response;
    }

    model::Game& game_;
};

}  // namespace http_handler