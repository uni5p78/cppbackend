#pragma once
#include "tick.h"
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <chrono>
// #include <boost/program_options.hpp>
// #include <boost/asio/strand.hpp>

// #include <fstream>
// #include <iostream>
// #include <optional>
// #include <vector>
// #include <memory>

// #include <boost/asio/strand.hpp>
// #include <boost/asio/ip/tcp.hpp>

// #include <memory>

// #include <boost/beast/core.hpp>
// #include <boost/beast/http.hpp>

namespace tick {

    
namespace net = boost::asio;
namespace sys = boost::system;
// namespace beast = boost::beast;
   
    
    
    class Ticker : public std::enable_shared_from_this<Ticker> {
    public:
        using Strand = net::strand<net::io_context::executor_type>;
        using Handler = std::function<void(std::chrono::milliseconds delta)>;
        // Функция handler будет вызываться внутри strand с интервалом period
        Ticker(Strand strand, std::chrono::milliseconds period, Handler handler);
        void Start() ;
    private:
        void ScheduleTick();
        void OnTick(sys::error_code ec);
        using Clock = std::chrono::steady_clock;
        Strand strand_;
        std::chrono::milliseconds period_;
        net::steady_timer timer_{strand_};
        Handler handler_;
        std::chrono::steady_clock::time_point last_tick_;
    };


} // namespace tick
