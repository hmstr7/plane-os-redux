#include "core.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "process.hpp"
#include <cstdlib>
#include <functional>
#include <memory>
#include <print>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include <thread>
#include <vector>
#include <signal.h>


Core::Core() {
    logger = spdlog::default_logger();
    logger->set_level(LOGGER_CORE_LEVEL);
}



void Core::init(Subprogram load_prog) {
    // TODO init
    core_state.fstate = FlightState::INIT;
    core_state.prog = load_prog;
}

void Core::process_manager() {
    // Add logging outputs
    shared_mem.sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
    shared_mem.sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(LOG_FILE));

    if (!(core_state.fstate == FlightState::INIT)) {
        // TODO handle uninitialized
        std::println("Core not initialized"); // TEMPORARY
        return;
    }

    switch (this->core_state.prog) {
        case IDLE: {
            // Add required processes
            procs.push_back(std::make_shared<IBUSProcess>(shared_mem));
            procs.push_back(std::make_shared<PWMProcess>(shared_mem));
            procs.push_back(std::make_shared<Middleman>(shared_mem));

            // Initialize processes
            for (auto& proc : procs) {
                auto init_res = proc->init();
                if (!init_res) {
                    // TODO advanced error handling
                    logger->error("Init failed for {}", typeid(proc).name());
                    return;
                }
            }
            
            // Config
            shared_mem.middleman_mode = STANDBY;
            shared_mem.mmode_user_allowed = false;

            // Launch processes
            std::vector<std::thread> threads;

            for (auto& proc : procs) {
                threads.push_back(std::thread(std::ref(*proc)));
            }

            for (auto& thread : threads) {
                thread.detach();
            }

            core_state.fstate = FlightState::OPERATIONAL;
        }
        case SIMPLE: {
            // Add required processes
            procs.push_back(std::make_shared<IBUSProcess>(shared_mem));
            procs.push_back(std::make_shared<PWMProcess>(shared_mem));
            procs.push_back(std::make_shared<Middleman>(shared_mem));

            // Initialize processes
            for (auto& proc : procs) {
                auto init_res = proc->init();
                if (!init_res) {
                    // TODO advanced error handling
                    logger->error("Init failed for {}", typeid(proc).name());
                    return;
                }
            }
            
            // Config
            shared_mem.middleman_mode = PASSTHROUGH;
            shared_mem.mmode_user_allowed = false;

            // Launch processes
            std::vector<std::thread> threads;

            for (auto& proc : procs) {
                threads.push_back(std::thread(std::ref(*proc)));
            }

            for (auto& thread : threads) {
                thread.detach();
            }

            core_state.fstate = FlightState::OPERATIONAL;
        }
        default:
            this->logger->critical("Subprogram is invalid or not implemented.");
    }
    

    // Watchdog loop
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

void Core::deinit() {
    logger->info("System shutting down...");

    core_state.fstate = DEINIT;

    if (!procs.empty()) {
        for (auto& proc : procs) {
            proc->stop();
        }
    }
}

Core::~Core() {
    deinit();
}
