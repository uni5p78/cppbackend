#include "logger.h"   
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

using namespace std::literals;

//---------------------------------------------------------------
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace json = boost::json;

namespace logger 
{

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // Момент времени     
    auto ts = *rec[timestamp];
    json::value log_record{{"timestamp"s, to_iso_extended_string(ts)}};

    // Добавляем данные в формате json, если есть
    auto data = rec[additional_data];
    if (data){
        log_record.as_object().emplace("data"s, *data);
    }

    // Добавляем само сообщение.
    log_record.as_object().emplace("message", *rec[logging::expressions::smessage]);

    // Выводим запись лога как объект json
    strm << log_record;
}

void InitLogFilter() {
    logging::add_common_attributes();
    logging::add_console_log(std::cout, logging::keywords::format = &MyFormatter,
                             logging::keywords::auto_flush = true);    
}

void LogServerStarted(int port, std::string address){
    json::value custom_data{{"port"s, port}, {"address"s, address}};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "server started"sv;
}

void LogExitFailure(const std::exception& ex){
    json::value custom_data{
          {"code"s, EXIT_FAILURE}
        , {"exception"s, ex.what()}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "server exited"sv;
}

void LogServerExited(){
    json::value custom_data{{"code"s, 0}};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "server exited"sv;
}

void LogException(const std::exception& ex, std::string_view where) {
    json::value custom_data{
          {"exception"s, ex.what()}
        , {"where"s, where}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "error"sv;
}

} // namespace logger