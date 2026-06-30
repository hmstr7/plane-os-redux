#pragma once
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "io.hpp"
#include <expected>
#include <memory>
#include <mutex>
#include "spdlog/common.h"
#include <spdlog/sinks/ansicolor_sink.h>
#include <vector>
#include "spdlog/spdlog.h"

#define LOGGER_PWM_NAME "PWM"
#define LOGGER_IBUS_NAME "IBUS"
#define LOGGER_PROCESS_LEVEL spdlog::level::trace

#define INIT_CLASS_LOGGER logger = spdlog::get(class_logger_name); \
    if (!logger) { \
        logger = std::make_shared<spdlog::logger>(class_logger_name, std::begin(shared_mem.sinks), std::end(shared_mem.sinks)); \
        spdlog::register_logger(logger); \
        logger->set_level(LOGGER_PROCESS_LEVEL); \
    }

enum ProcessState {
    NOT_RUNNING,
    RUNNING,
    FAILURE,
    STOPPED
};

struct Shared {
    /*
    private:
        std::mutex q_ctrl_lock{};
        control_queue_t q_ctrl;
    inline control_queue_t* get_q_ctrl() {   
    };
    */
    control_queue_t q_ctrl;
    std::vector<spdlog::sink_ptr> sinks;
};

class Process {
    protected:
        const char* class_logger_name;
        std::shared_ptr<spdlog::logger> logger;
        Shared& shared_mem;
        bool initialized = false;
    public:
        inline Process(Shared& shared_mem): shared_mem(shared_mem) {};
        ProcessState state = ProcessState::NOT_RUNNING;
        virtual void_expensive init() = 0;
        virtual void operator()() = 0;
        virtual void stop() = 0;
};


class PWMProcess : public Process {
    private:
        std::unique_ptr<PWM> pwm_board;
    public:
        inline PWMProcess(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};

class IBUSProcess :public Process {
   private:
        std::unique_ptr<IBUS> ibus;
        ibus_context_t ctx;
        uart_lowlevel_config config;
      
        static void handler(ibus_channel_t* channels_raw, void* cookie);;
   public:
        inline IBUSProcess(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};
