#pragma once

#include <boost/program_options.hpp>
#include <optional>
#include <chrono>


namespace comand_line {

struct Args {
    int tick_period{};
    std::string config_file{};
    std::string www_root{};
    bool randomize_spawn_points{};
}; 


[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) ;

} // namespace comand_line 

