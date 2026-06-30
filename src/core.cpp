#include "core.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "process.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <ratio>
#include <thread>
#include <vector>


void Core::init() {
    // TODO init
    core_state.fstate = FlightState::INIT;
}

void Core::process_manager() {
    Shared shared_mem;

    if (!(core_state.fstate == FlightState::INIT)) {
        // TODO handle uninitialized
        return;
    }

    // initialize Process objects
    switch (core_state.mode) {
        case FlightMode::SIMPLE: {
            std::vector<std::unique_ptr<Process>> procs;

            // Add required processes
            procs.push_back(std::make_unique<IBUSProcess>(shared_mem));
            procs.push_back(std::make_unique<PWMProcess>(shared_mem));

            for (auto& proc : procs) {
                auto init_res = proc->init();
                if (!init_res) {
                    // TODO handle failed init
                    return;
                }
            }
            
            std::vector<std::thread> threads;

            for (auto& proc : procs) {
                threads.push_back(std::thread(std::ref(*proc)));
            }

            for (auto& thread : threads) {
                thread.detach();
            }

            core_state.fstate = FlightState::OPERATIONAL;

            while (core_state.fstate == FlightState::OPERATIONAL) {
                for (auto& proc : procs) {
                    if (proc->state == ProcessState::FAILURE) {
                        // TODO handle failed process here
                        core_state.fstate = FlightState::DEINIT;
                        break;
                    } else if (proc->state == ProcessState::STOPPED) {
                        // TODO handle stopped process here
                        core_state.fstate = FlightState::DEINIT;
                        break;
                    }
                }
                std::this_thread::sleep_for(PROCESS_MGR_SLEEP);
            }
            // Deinit
            deinit();
        }
        default:
            // TODO not implemented
            return;
    }
}

void Core::deinit() {
    // deinit logic here
}

Core::~Core() {
    deinit();
}
