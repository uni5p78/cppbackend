#pragma once

#include "request_handler.h"
#include "logger.h"   
#include <boost/beast/http.hpp>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;
using namespace std::chrono;
using StringRequest = http::request<http::string_body>;



namespace log_handler{

class LoggingRequestHandler {
public:

    LoggingRequestHandler(http_handler::RequestHandler& request_handler);

    template <typename Body, typename Allocator, typename Send>
    void operator()(std::string&& end_point, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        LogRequest(end_point, req);

        const auto start_time = steady_clock::now();
        auto sender = [send = std::forward<Send>(send), self = this, start_time](auto&& res){
            self->LogResponse(res, start_time);
            send(std::forward<decltype(res)>(res));
        };

        // Обработать запрос request и отправить ответ, используя send
        true_handler_(std::forward<decltype(req)>(req), std::move(sender));
    }


private:
    http_handler::RequestHandler& true_handler_;

    static std::string GetNameMetod(http::verb method);
    static void LogRequest(std::string end_point, const StringRequest& req);

    template <typename Body>
    void LogResponse(const http::response<Body>& response, const std::chrono::_V2::steady_clock::time_point start_time){
        const auto duration = std::chrono::steady_clock::now() - start_time;
        json::value custom_data{
              {"response_time"s, duration_cast<std::chrono::milliseconds>(duration).count()}
            , {"code"s, response.result_int()} // http::response<T>::result_int()
            , {"content_type"s, ""s}
        };
        if (response.base().count(http::field::content_type) == 0){
            custom_data.as_object().at("content_type"s) = nullptr;           
        } else {
            custom_data.as_object().at("content_type"s) = std::string(response.base().at(http::field::content_type));         
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                << "response sent"sv;
    }

};

}// namespace logging_request_handler