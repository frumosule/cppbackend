#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

class SessionBase {
public:
    virtual ~SessionBase() = default;
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& handler)
        : stream_(std::move(socket))
        , handler_(std::forward<Handler>(handler)) {
    }

    void Run() {
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(&Session::Read, this->shared_from_this()));
    }

private:
    void Read() {
        request_ = {};
        stream_.expires_after(std::chrono::seconds(30));

        http::async_read(stream_, buffer_, request_,
                         beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
    }

    void OnRead(beast::error_code ec, std::size_t bytes_read) {
        if (ec == http::error::end_of_stream) {
            return Close();
        }
        if (ec) {
            return;
        }

        HandleRequest();
    }

    void HandleRequest() {
        auto self = this->shared_from_this();
        
        // Используем strand для гарантии, что обработчик не будет вызван конкурентно
        net::dispatch(stream_.get_executor(), [self] {
            self->handler_(std::move(self->request_),
                          [self](StringResponse&& response) {
                self->Write(std::move(response));
            });
        });
    }

    void Write(StringResponse&& response) {
        auto sp = std::make_shared<StringResponse>(std::move(response));

        http::async_write(stream_, *sp,
                          beast::bind_front_handler(&Session::OnWrite, this->shared_from_this(), sp->need_eof()));
    }

    void OnWrite(bool close, beast::error_code ec, std::size_t bytes_written) {
        if (ec) {
            return;
        }

        if (close) {
            return Close();
        }

        Read();
    }

    void Close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

    beast::tcp_stream stream_;
    RequestHandler handler_;
    beast::flat_buffer buffer_;
    StringRequest request_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, tcp::endpoint endpoint, Handler&& handler)
        : ioc_(ioc)
        , acceptor_(ioc)
        , handler_(std::forward<Handler>(handler)) {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            return;
        }

        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            return;
        }

        acceptor_.bind(endpoint, ec);
        if (ec) {
            return;
        }

        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            return;
        }
    }

    void Run() {
        DoAccept();
    }

private:
    void DoAccept() {
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            return;
        }

        std::make_shared<Session<RequestHandler>>(std::move(socket), handler_)->Run();
        DoAccept();
    }

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    std::make_shared<Listener<std::decay_t<RequestHandler>>>(
        ioc, endpoint, std::forward<RequestHandler>(handler))
        ->Run();
}

}  // namespace http_server