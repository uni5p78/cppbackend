#include "sdk.h"
//
// #include <boost/program_options.hpp>
// #include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
// #include <boost/log/trivial.hpp>
// #include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "tick.h"
#include "comand_line.h"  

#include "boost_log.h"   
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
namespace json = boost::json;
namespace logging = boost::log;

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}


}  // namespace

int main(int argc, const char* argv[]) {
    // if (argc != 3) {
    //     std::cerr << "Usage: game_server <game-config-json> <path-static-files>"sv << std::endl;
    //     return EXIT_FAILURE;
    // }
    try {
        boost_log::InitBoostLogFilter();
        auto args = comand_line::ParseCommandLine(argc, argv);
        if (!args) {
            return EXIT_SUCCESS;
        }

        // std::string config_file(argv[1]);
        // std::string static_content_path(argv[2]);

        // std::string config_file("../../data/config.json");
        // std::string static_content_path("../../static");

        std::string config_file(args->config_file);
        std::string static_content_path(args->www_root);

        // 1. Загружаем карту из файла и строим модель игры
        model::Game game(args->randomize_spawn_points);
        json_loader::LoadGame(game, config_file);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {ioc.stop();}
        });
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры через объект с сценариями игры (application)
        // strand для выполнения запросов к API
        app::Application application(game);
        auto api_strand = net::make_strand(ioc);
        const bool is_test_tick_mode = args->tick_period == 0; 
        auto handler = std::make_shared<http_handler::RequestHandler>(application, static_content_path, api_strand, is_test_tick_mode);
        // http_handler::RequestHandler handler{game, static_content_path, api_strand};
        log_handler::LoggingRequestHandler logging_handler{*handler};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& end_point, auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(end_point)>(end_point), std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // 6. Настраиваем вызов метода Application::Tick каждые хх миллисекунд внутри strand
        if(!is_test_tick_mode){
            auto ticker = std::make_shared<tick::Ticker>(api_strand, std::chrono::milliseconds(args->tick_period),
                [&application](std::chrono::milliseconds delta) { application.ChangeGameSate(delta); }
            );
            ticker->Start();
        }


        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        // std::cout << "Server has started..."sv << std::endl;
        boost_log::LogServerStarted(port, address.to_string());

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        // std::cerr << ex.what() << std::endl;
        boost_log::LogExitFailure(ex); 
        return EXIT_FAILURE;
    }
    boost_log::LogServerExited();

}
