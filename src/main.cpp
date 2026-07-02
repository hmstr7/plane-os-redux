#include "common.hpp"
#include "core.hpp"
#include <signal.h>
#include "spdlog/spdlog.h"

struct sigint_handler {
    static Core* self;
    static void operator()(int s) {
        spdlog::default_logger()->info("Interrupt");
        self->deinit();
        exit(1);
    } 
};



int main() {
    // struct sigaction sigIntHandler;

    // sigint_handler interrupt_handler{};

    // sigIntHandler.sa_handler = interrupt_handler;
    // sigemptyset(&sigIntHandler.sa_mask);
    // sigIntHandler.sa_flags = 0;

    // sigaction(SIGINT, &sigIntHandler, NULL);

    Core flight_core{};

    flight_core.core_state.mode = FlightMode::SIMPLE;
    flight_core.init();
    flight_core.process_manager();
    
    return 0;
}