#pragma once
#include "common.hpp"
#include <memory>
#include "spdlog/spdlog.h"

#define PROCESS_MGR_SLEEP std::chrono::milliseconds(500)
#define LOG_FILE "../logs/blackbox.log"

#define LOGGER_CORE_LEVEL spdlog::level::info

class Core {
    private:
        control_queue_t q_ctrl = std::queue<Control>();
        std::shared_ptr<spdlog::logger> logger;
    public:
        State core_state;
        
        Core();

        void init();
        void process_manager();
        void deinit();

        ~Core();
};