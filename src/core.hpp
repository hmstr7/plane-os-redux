#pragma once
#include "common.hpp"

#define PROCESS_MGR_SLEEP std::chrono::milliseconds(500)

class Core {
    private:
        control_queue_t q_ctrl = std::queue<Control>();
    public:
        State core_state;
        
        void init();
        void process_manager();
        void deinit();

        ~Core();
};