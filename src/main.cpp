#include "common.hpp"
#include "core.hpp"
#include <iostream>
#include <signal.h>
#include "spdlog/spdlog.h"

Core flight_core{};

extern "C" void handle_signal(int signal)
{
    spdlog::default_logger()->info("Interrupt");
    ::flight_core.deinit();
    exit(1);
}

int main() {
    struct sigaction sa {};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1 ||
        sigaction(SIGTERM, &sa, nullptr) == -1) {
        spdlog::default_logger()->critical("Failed to install signal handlers\n");
        return EXIT_FAILURE;
    }

    flight_core.init(Subprogram::SIMPLE);
    flight_core.process_manager();
    
    return 0;
}