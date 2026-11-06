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

#define JSONGET(config_addr, to_var, to_var_t, ...) \
std::expected<to_var_t, Error> to_var = get_from_json<to_var_t>(config_addr, {__VA_ARGS__}); \
if (!to_var) { \
    return std::unexpected(to_var.error()); \
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

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)

#define SAVEJSON(config_addr, val, val_t, ...) \
write_to_json<val_t>(config_addr, {__VA_ARGS__}, val).transform_error([&fail, &last_err](const Error& err) {fail = true; last_err = err; return err;});


/**
 * @brief Enum representing current state of a process
 * 
 * @todo Documentation
 */
enum ProcessState {
    NOT_READY,
    READY,
    RUNNING,
    FAILED,
    STOPPED
};

/**
 * @brief Enum representing current mode of operation of the middle man
 * 
 * @todo Documentation
 */
enum MiddlemanMode {
    NOT_SET,
    STANDBY,
    PASSTHROUGH,
    RATE_STAB
};


/**
 * Shared memory of all processes
 * 
 * @details This struct contains varaiables accessible (and sometimes modifiable) by all processes, as well as some flags pre-set by the @ref FlightCore::process_manager or 
 * set dynamically
 */
struct Shared {
    Control c_input = CONTROL_NEUTRAL; std::mutex c_input_mut; /** Register containing last received control sequence */
    Control c_output = CONTROL_NEUTRAL; std::mutex c_output_mut; /** Register containing control sequence that needs to be executed */
    RatesRead c_imu = RatesRead(0,0,0); std::mutex c_imu_mut; /** Register containing last IMU reading (gyro) */

    std::chrono::time_point<std::chrono::steady_clock> flightlog_starttime; /** Point in time that is considered the start of the flight log */

    // Flags
    bool f_logflightdata = false; /** If flight data needs to be logged into a file (currently includes: received inputs, imu readings) */
    bool f_prompt_calibrate = true; /** By default, the user is prompted to calibrate the IMU. This behaviour can be overriden by supplying --no-calib option. */
    bool f_should_calibrate = true; /** If IMU calibration needs to be performed. */

    MiddlemanMode middleman_mode = STANDBY; /** Current mode of operation of the Middleman process */
    bool mmode_user_allowed = false; /** Current middleman state is allowed to be influenced by the user */

    std::vector<spdlog::sink_ptr> sinks; /** spdlog sinks (ex. console output or log file) */
};


/**
 * @name Processes
 * @{
 */

/**
 * @brief A process is an object running a routine responsible for managing one task (such as receiving commands). 
 * Its operator()() function is what is being run in parallel along with other processes.
 */
class Process {
    protected:
        const char* class_logger_name;
        std::shared_ptr<spdlog::logger> logger; /** Logger specific to this process, created in @ref Process::init() and is referred to by a name defined as LOGGER_<class name>_NAME in process.hpp */
        Shared& shared_mem;
    public:
        inline Process(Shared& shared_mem): shared_mem(shared_mem) {};
        ProcessState state = ProcessState::NOT_READY;
        virtual void_expensive init() = 0;
        virtual void operator()() = 0;
        virtual void stop() = 0;
};

/**
 * @brief Process resposible for controlling a @ref PWM object and executing commands from @ref Shared::c_output
 */
class PWMProcess : public Process {
    private:
        std::unique_ptr<PWM> pwm_board;
    public:
        PWMProcess(Shared& shared_mem);
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};

/**
 * @brief Process resposible for controlling an @ref IBUS object and writing received control sequences to @ref Shared::c_input
 */
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

/**
 * @brief Process resposible for controlling an @ref IMU object and writing read gyro values to @ref Shared::c_imu
 */
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


/**
 * @brief Process responsible for processing input data and producing output
 * 
 * @details Middleman currently has multiple modes:
 * * @ref MiddlemanMode::STANDBY - do nothing
 * * @ref MiddlemanMode::PASSTHROUGH - move the control sequences from @ref Shared::c_input to @ref Shared::c_output without modifying them
 * * @ref MiddlemanMode::RATE_STAB - (not implemented)
 */
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

/**
 * @}
 */