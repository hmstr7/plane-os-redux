#include "common.hpp"
#include "core.hpp"

int main() {
    Core flight_core{};
    
    flight_core.core_state.mode = FlightMode::SIMPLE;
    flight_core.process_manager();

    return 0;
}