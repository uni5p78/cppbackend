#include "logging_request_handler.h"


namespace log_handler
{
    LoggingRequestHandler::LoggingRequestHandler(http_handler::RequestHandler& request_handler)
    : true_handler_(request_handler)
    {}

    std::string LoggingRequestHandler::GetNameMetod(http::verb method){
        switch(method){
        case http::verb::get: 
            return "GET"s;
        case http::verb::head: 
            return "HEAD"s;
        default: 
            return "UNKNOWN"s;
        }
    }

    void LoggingRequestHandler::LogRequest(std::string end_point, const StringRequest& req){
        auto target = req.target();
        std::string uri(target.data(), target.size());
        json::value custom_data{
              {"ip"s, end_point}
            , {"URI"s, uri}
            , {"method"s, GetNameMetod(req.method())}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                << "request received"sv;
    }



}// namespace logging_request_handler