#include "core.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "process.hpp"
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
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
    spdlog::set_error_handler([](const std::string& msg) {
        std::cerr << "spdlog error: " << msg << std::endl;
    });
}



void Core::init(Subprogram load_prog, bool logflightdata = false, bool prompt_calibrate = true, bool should_calibrate = true) {
    // TODO init
    core_state.fstate = FlightState::INIT;
    core_state.prog = load_prog;
    pf_logflightdata = logflightdata;
    pf_prompt_calib = prompt_calibrate;
    pf_should_calib = should_calibrate;
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
            this->logger->trace("Loading IDLE program");
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
            break;
        }
        case SIMPLE: {
            this->logger->trace("Loading SIMPLE program");
            // Add required processes
            procs.push_back(std::make_shared<IBUSProcess>(shared_mem));
            procs.push_back(std::make_shared<PWMProcess>(shared_mem));
            procs.push_back(std::make_shared<Middleman>(shared_mem));

            // Initialize processes
            for (auto& proc : procs) {
                auto init_res = proc->init();
                if (!init_res) {
                    // TODO advanced error handling
                    logger->error("Init failed for {}", typeid(*proc).name());
                    return;
                }
            }

            // Flags
            shared_mem.f_logflightdata = pf_logflightdata;
            
            // Config
            shared_mem.middleman_mode = PASSTHROUGH;
            shared_mem.mmode_user_allowed = false;
            shared_mem.flightlog_starttime = std::chrono::steady_clock::now();
            

            // Launch processes
            std::vector<std::thread> threads;
            for (auto& proc : procs) {
                threads.push_back(std::thread(std::ref(*proc)));
            }

            for (auto& thread : threads) {
                thread.detach();
            }

            core_state.fstate = FlightState::OPERATIONAL;
            break;
        }
        default:
            this->logger->critical("Subprogram is invalid or not implemented.");
            return;
    }
    

    // Watchdog loop
    while (core_state.fstate == FlightState::OPERATIONAL) {
        for (auto& proc : procs) {
            if (proc->state == ProcessState::FAILED) {
                // TODO handle failed process here
                logger->error("Process {} failed.", typeid(*proc).name());  // TEMPORARY
                break;
            } else if (proc->state == ProcessState::STOPPED) {
                logger->info("Process {} stopped.", typeid(*proc).name());
                // TODO handle stopped process here
                break;
            }
        }
        std::this_thread::sleep_for(PROCESS_MGR_SLEEP);
    }
    deinit();
}

void Core::deinit() {
    logger->info("System shutting down...");

    core_state.fstate = DEINIT;

    if (!procs.empty()) {
        for (auto& proc : procs) {
            if (proc) {
                proc->stop();
            }
        }
    }
    logger->info("Bye");
}

Core::~Core() {
    deinit();
}
