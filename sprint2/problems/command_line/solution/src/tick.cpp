#include "tick.h"
#include "logger.h"   

namespace tick {

    
namespace net = boost::asio;
namespace sys = boost::system;
using namespace std::literals;
   
    
    
// Функция handler будет вызываться внутри strand с интервалом period
Ticker::Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
    : strand_{strand}
    , period_{period}
    , handler_{std::move(handler)} {
}

void Ticker::Start() {
    net::dispatch(strand_, [self = shared_from_this()] {
        self->last_tick_ = Clock::now();
        self->ScheduleTick();
    });
}

void Ticker::ScheduleTick() {
    assert(strand_.running_in_this_thread());
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);
    });
}

void Ticker::OnTick(sys::error_code ec) {
    using namespace std::chrono;
    assert(strand_.running_in_this_thread());

    if (!ec) {
        auto this_tick = Clock::now();
        auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
        last_tick_ = this_tick;
        try {
            handler_(delta);
        } catch (const std::exception& ex) {
            logger::LogException(ex, "Ticker::OnTick handler_(delta)"sv);
        }
        ScheduleTick();
    }
}

using Clock = std::chrono::steady_clock;



} // namespace tick
