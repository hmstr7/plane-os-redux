#include "common.hpp"
#include "core.hpp"
#include <iostream>
#include <signal.h>
#include "spdlog/spdlog.h"
#include <boost/program_options.hpp>
namespace po = boost::program_options;

Core flight_core{};

extern "C" void handle_signal(int signal)
{
    spdlog::default_logger()->info("Interrupt");
    ::flight_core.deinit();
    exit(1);
}

int main(int ac, char * av[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("flightlog", "log inputs into a flight log file")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);    

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    struct sigaction sa {};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (
        sigaction(SIGINT, &sa, nullptr) == -1 ||
        sigaction(SIGTERM, &sa, nullptr) == -1
    ) {
        spdlog::default_logger()->critical("Failed to install signal handlers\n");
        return EXIT_FAILURE;
    }

    flight_core.init(Subprogram::SIMPLE, vm.count("flightlog"));
    flight_core.process_manager();
    
    return 0;
}