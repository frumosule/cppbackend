#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <functional>

namespace net = boost::asio;

class Ticker {
public:
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    Ticker(net::io_context& ioc, 
           std::chrono::milliseconds period, 
           Handler handler);
    
    void Start();
    void Stop();

private:
    void ScheduleTick();
    void OnTick(boost::system::error_code ec);

    net::steady_timer timer_;
    std::chrono::milliseconds period_;
    Handler handler_;
    bool active_ = false;
    std::chrono::steady_clock::time_point last_tick_;
};