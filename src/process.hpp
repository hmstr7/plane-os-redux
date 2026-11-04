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
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"

#define LOGGER_PWM_NAME "PWM"
#define LOGGER_IBUS_NAME "IBUS"
#define LOGGER_IMU_NAME "IMU"
#define LOGGER_MIDDLEMAN_NAME "MIDDLEMAN"
#define LOGGER_PROCESS_LEVEL spdlog::level::trace
#define Q_IN_SIZE 100
#define Q_OUT_SIZE 100

#define JSONGET(arg_config_jg,var,T, ...) \
auto var = get_from_json<T>(arg_config_jg.value(), {__VA_ARGS__}); \
if (!var) { \
    return std::unexpected(var.error()); \
}

#define JSONGET_BIAS(arg_config_jgb,var,T, ...) \
auto var = get_from_json<T>(arg_config_jgb.value(), {__VA_ARGS__}); \
if (!var) { \
    logger->error("Failed to get bias from config - {}", var.error()); \
    fail = true; \
}

#define TO_UINT8(configv,var) \
auto var = to_uint8(configv.value()); \
if (!var) { \
    return std::unexpected(Error(var.error())); \
}

#define INIT_CLASS_LOGGER \
logger = spdlog::get(class_logger_name); \
if (!logger) { \
    logger = std::make_shared<spdlog::logger>(class_logger_name, std::begin(shared_mem.sinks), std::end(shared_mem.sinks)); \
    spdlog::register_logger(logger); \
    logger->set_level(LOGGER_PROCESS_LEVEL); \
}

#define SAVEBIAS(arg_config_sb,var,k,ind) \
auto var = write_to_json<float>(arg_config_sb.value(), CONFIG_PATH_AUTO, {k}, imu->getBias()[ind]); \
if (!res) { \
    fail = true; \
}


enum ProcessState {
    NOT_READY,
    READY,
    RUNNING,
    FAILED,
    STOPPED
};

enum MiddlemanMode {
    NOT_SET,
    STANDBY,
    PASSTHROUGH,
    RATE_STAB
};

struct Shared {
    Control c_input = CONTROL_NEUTRAL; std::mutex c_input_mut; // Register containing last received control sequence
    Control c_output = CONTROL_NEUTRAL; std::mutex c_output_mut; // Register containing control sequence that needs to be executed
    RatesRead c_imu = RatesRead(0,0,0); std::mutex c_imu_mut; // Register containing last IMU reading (gyro)

    std::chrono::time_point<std::chrono::steady_clock> flightlog_starttime; // Point in time that is considered the start of the flight log

    // Flags
    bool f_logflightdata = false; // If flight data needs to be logged into a file (currently includes: received inputs, imu readings)
    bool f_prompt_calibrate = true; // By default, the user is prompted to calibrate the IMU. This behaviour can be overriden by supplying --no-calib option.
    bool f_should_calibrate = true; // If IMU calibration needs to be performed.

    MiddlemanMode middleman_mode = STANDBY; // Current mode of operation of the Middleman process 
    bool mmode_user_allowed = false; // Current middleman state is allowed to be influenced by the user

    std::vector<spdlog::sink_ptr> sinks; // spdlog sinks (ex. console output or log file)
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
    private:
        std::unique_ptr<IMU> imu;
        std::shared_ptr<spdlog::logger> imu_log;
    public:
        IMUProcess(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};

class Middleman: public Process {
    private:
        bool running = false;
        Control ctrl_buff = CONTROL_NEUTRAL;
        MiddlemanMode lastm = NOT_SET;
    public:
        Middleman(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};
