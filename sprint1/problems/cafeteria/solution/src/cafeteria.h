#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
namespace sys = boost::system;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class HotDogOrder : public std::enable_shared_from_this<HotDogOrder> {
public:
    HotDogOrder(net::io_context& io, Store& store, std::shared_ptr<GasCooker> gas_cooker, 
                HotDogHandler handler)
        : io_(io)
        , store_(store)
        , gas_cooker_(std::move(gas_cooker))
        , handler_(std::move(handler)) {
    }

    void Start() {
        auto self = shared_from_this();
        
        // Сначала готовим сосиску, потом булку
        PrepareSausage([self] {
            self->PrepareBread();
        });
    }

private:
    void PrepareSausage(std::function<void()> callback) {
        sausage_ = store_.GetSausage();
        
        auto self = shared_from_this();
        sausage_->StartFry(*gas_cooker_, [self, callback] {
            self->sausage_timer_.emplace(self->io_, Milliseconds{1500});
            self->sausage_timer_->async_wait([self, callback](sys::error_code ec) {
                if (!ec) {
                    self->sausage_->StopFry();
                    callback();
                } else {
                    self->handler_(Result<HotDog>{std::make_exception_ptr(
                        std::runtime_error("Sausage cooking failed"))});
                }
            });
        });
    }

    void PrepareBread() {
        bread_ = store_.GetBread();
        
        auto self = shared_from_this();
        bread_->StartBake(*gas_cooker_, [self] {
            self->bread_timer_.emplace(self->io_, Milliseconds{1000});
            self->bread_timer_->async_wait([self](sys::error_code ec) {
                if (!ec) {
                    self->bread_->StopBaking();
                    self->AssembleHotDog();
                } else {
                    self->handler_(Result<HotDog>{std::make_exception_ptr(
                        std::runtime_error("Bread baking failed"))});
                }
            });
        });
    }

    void AssembleHotDog() {
        try {
            static int next_hotdog_id = 1;
            HotDog hotdog{next_hotdog_id++, sausage_, bread_};
            handler_(Result<HotDog>{std::move(hotdog)});
        } catch (const std::exception& e) {
            handler_(Result<HotDog>{std::make_exception_ptr(e)});
        }
    }

    net::io_context& io_;
    Store& store_;
    std::shared_ptr<GasCooker> gas_cooker_;
    HotDogHandler handler_;
    std::shared_ptr<Bread> bread_;
    std::shared_ptr<Sausage> sausage_;
    std::optional<net::steady_timer> sausage_timer_;
    std::optional<net::steady_timer> bread_timer_;
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        auto order = std::make_shared<HotDogOrder>(io_, store_, gas_cooker_, std::move(handler));
        order->Start();
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};