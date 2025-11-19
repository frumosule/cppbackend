#include "ticker.h"
#include <iostream>

Ticker::Ticker(net::io_context& ioc,
               std::chrono::milliseconds period, 
               Handler handler)
    : timer_(ioc)
    , period_(period)
    , handler_(std::move(handler))
    , last_tick_(std::chrono::steady_clock::now()) {
}

void Ticker::Start() {
    active_ = true;
    ScheduleTick();
}

void Ticker::Stop() {
    active_ = false;
    timer_.cancel();
}

void Ticker::ScheduleTick() {
    if (!active_) {
        return;
    }

    timer_.expires_after(period_);
    timer_.async_wait([this](boost::system::error_code ec) {
        OnTick(ec);
    });
}

void Ticker::OnTick(boost::system::error_code ec) {
    if (ec) {
        if (ec != net::error::operation_aborted) {
            std::cerr << "Ticker error: " << ec.message() << std::endl;
        }
        return;
    }

    if (!active_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_);
    last_tick_ = now;

    try {
        handler_(delta);
    } catch (const std::exception& ex) {
        std::cerr << "Exception in tick handler: " << ex.what() << std::endl;
    }

    ScheduleTick();
}