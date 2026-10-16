#pragma once
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "io.hpp"
#include <chrono>
#include <expected>
#include <memory>
#include <mutex>
#include "spdlog/common.h"
#include <vector>
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

#define LOGGER_PWM_NAME "PWM"
#define LOGGER_IBUS_NAME "IBUS"
#define LOGGER_IMU_NAME "IMU"
#define LOGGER_MIDDLEMAN_NAME "MIDDLEMAN"
#define LOGGER_PROCESS_LEVEL spdlog::level::trace
#define Q_IN_SIZE 100
#define Q_OUT_SIZE 100

#define INIT_CLASS_LOGGER logger = spdlog::get(class_logger_name); \
    if (!logger) { \
        logger = std::make_shared<spdlog::logger>(class_logger_name, std::begin(shared_mem.sinks), std::end(shared_mem.sinks)); \
        spdlog::register_logger(logger); \
        logger->set_level(LOGGER_PROCESS_LEVEL); \
    }

enum ProcessState {
    NOT_READY,
    READY,
    RUNNING,
    FAILURE,
    STOPPED
};

enum MiddlemanMode {
    NOT_SET,
    STANDBY,
    PASSTHROUGH,
    RATE_STAB
};

struct Shared {
    Control c_input = CONTROL_NEUTRAL; std::mutex c_input_mut;
    Control c_output = CONTROL_NEUTRAL; std::mutex c_output_mut;

    std::chrono::time_point<std::chrono::steady_clock> flightlog_starttime;

    // Flags
    bool f_logflightdata = false;

    MiddlemanMode middleman_mode = STANDBY;
    bool mmode_user_allowed = false;

    std::vector<spdlog::sink_ptr> sinks;
};

class Process {
    protected:
        const char* class_logger_name;
        std::shared_ptr<spdlog::logger> logger;
        Shared& shared_mem;
    public:
        inline Process(Shared& shared_mem): shared_mem(shared_mem) {};
        ProcessState state = ProcessState::NOT_READY;
        virtual void_expensive init() = 0;
        virtual void operator()() = 0;
        virtual void stop() = 0;
};


class PWMProcess : public Process {
    private:
        std::unique_ptr<PWM> pwm_board;
    public:
        PWMProcess(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};

class IBUSProcess: public Process {
   private:
        std::unique_ptr<IBUS> ibus;
        ibus_context_t ctx;
        uart_lowlevel_config config;

        static void handler(ibus_channel_t* channels_raw, void* cookie);

        std::shared_ptr<spdlog::logger> input_log;
   public:
        IBUSProcess(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};

class IMUProcess: public Process {
    
};

class Middleman: public Process {
    private:
        bool running = false;
        Control ctrl_buff = Control(0,0,0,0);
        MiddlemanMode lastm = NOT_SET;
    public:
        Middleman(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};
