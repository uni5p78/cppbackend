#include "comand_line.h"  
#include <iostream>


namespace comand_line {
using namespace std::literals;


[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"All options"s};
    Args args;
    // Добавляем опцию --help и её короткую версию -h
    desc.add_options()
        ("help,h", "Show help")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.www_root)->value_name("path"s), "set static files root")
        ("tick-period,t", po::value(&args.tick_period)->value_name("ms"s), "set tick period")
        ("randomize-spawn-points", po::value(&args.randomize_spawn_points), "spawn dogs at random positions")
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file have not been specified!"s);
    }

    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files have not been specified!"s);
    }

    return args;
}

} // namespace comand_line 

