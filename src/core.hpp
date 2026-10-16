#pragma once
#include "common.hpp"
#include <memory>
#include "process.hpp"
#include "spdlog/spdlog.h"

#define PROCESS_MGR_SLEEP std::chrono::milliseconds(500)
#define LOG_FILE "../logs/blackbox.log"

#define LOGGER_CORE_LEVEL spdlog::level::trace

class Core {
    private:
        Shared shared_mem;
        std::vector<std::shared_ptr<Process>> procs;
        std::shared_ptr<spdlog::logger> logger;
        bool pf_logflightdata;
    public:
        State core_state;
        
        Core();

        void init(Subprogram load_prog, bool logflightdata);
        void process_manager();
        void deinit();

        ~Core();
};