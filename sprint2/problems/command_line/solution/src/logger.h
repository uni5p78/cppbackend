#pragma once

#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
namespace json = boost::json;
namespace logging = boost::log;
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

namespace logger 
{
    void InitLogFilter();
    void LogServerStarted(int port, std::string address);
    void LogExitFailure(const std::exception& ex);
    void LogServerExited();
    void LogException(const std::exception& ex, std::string_view where);

} // namespace logger