#include "core.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "process.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <print>
#include <spdlog/sinks/ansicolor_sink.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/spdlog-inl.h>
#include <thread>
#include <vector>

Core::Core() {
    logger = spdlog::default_logger();
    logger->set_level(LOGGER_CORE_LEVEL);
}

void Core::init() {
    // TODO init
    ;
    core_state.fstate = FlightState::INIT;
}

void Core::process_manager() {
    Shared shared_mem;

    // Add logging outputs
    shared_mem.sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
    shared_mem.sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(LOG_FILE));

    if (!(core_state.fstate == FlightState::INIT)) {
        // TODO handle uninitialized
        std::println("Core not initialized"); // TEMPORARY
        return;
    }

    // initialize Process objects
    switch (core_state.mode) {
        case FlightMode::SIMPLE: {
            std::vector<std::shared_ptr<Process>> procs;

            // Add required processes
            procs.push_back(std::make_shared<IBUSProcess>(shared_mem));
            procs.push_back(std::make_shared<PWMProcess>(shared_mem));

            for (auto& proc : procs) {
                auto init_res = proc->init();
                if (!init_res) {
                    // TODO advanced error handling
                    logger->error("Init failed for {}", typeid(proc).name());
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
                        logger->error("Process {} failed.", typeid(proc).name());  // TEMPORARY
                        core_state.fstate = FlightState::DEINIT;
                        break;
                    } else if (proc->state == ProcessState::STOPPED) {
                        logger->info("Process {} stopped.", typeid(proc).name());
                        // TODO handle stopped process here
                        core_state.fstate = FlightState::DEINIT;
                        break;
                    }
                }
                std::this_thread::sleep_for(PROCESS_MGR_SLEEP);
            }
        }
        default:
            // TODO handle not implemented
            logger->critical("Flight mode not implemented"); // TEMPORARY
    }
    deinit();
}

void Core::deinit() {
    // deinit logic here
    logger->debug("Core deinit"); // TEMPORARY
}

Core::~Core() {
    deinit();
}
