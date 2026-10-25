#include "io.hpp"
#include <cstdint>
#include "PiPCA9685/PCA9685.h"
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <expected>
#include <iomanip>
#include <memory>
#include <string>
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "process.hpp"
extern "C" {
    #include "ibus/ibus.h"
    #include "../extern/sensor-imu/imu.h"
}

/*
Initialize PCA9685 (access through private `board` field)
*/
PWM::PWM(
    std::string device,
    uint8_t ele_ch,
    uint8_t l_ail_ch,
    uint8_t r_ail_ch,
    uint8_t l_thr_ch,
    uint8_t r_thr_ch
) : device(device), ele_ch(ele_ch), l_ail_ch(l_ail_ch), r_ail_ch(r_ail_ch), l_thr_ch(l_thr_ch), r_thr_ch(r_thr_ch) {
    logger = spdlog::get(LOGGER_PWM_NAME);
    if (!logger) {
        logger = spdlog::default_logger(); // TODO proper fallback
    }
    
    board = std::make_unique<PiPCA9685::PCA9685>(device);
    board->set_pwm_freq(PWM_FREQ);
    logger->debug("PWM io module init done");
}

/*
Executes given control sequence.
* Warning: performance critical!
*/
void_cheap PWM::execute(Control& control_sequence) {
    // TODO temporary fix
    if (control_sequence.elevator == 0 || control_sequence.aileron == 0 || control_sequence.throttle == 0 || control_sequence.s_arm == 0) {
	logger->warn("Invalid Control, dropping");
        return {};
    }
    static uint16_t s_arm_buff = 0;

    if (control_sequence.s_arm > SWITCH_MIDDLE) {
        if (control_sequence.s_arm != s_arm_buff) {
          logger->info("Armed");
        }
        
        double throttle = (double) control_sequence.throttle/1000;
        board->set_pwm_ms(l_thr_ch, throttle);
        board->set_pwm_ms(r_thr_ch, throttle);
    } else {
        if (control_sequence.s_arm != s_arm_buff) {
            logger->info("Disarmed");
        }
        board->set_pwm_ms(l_thr_ch, (double) DISARMED_THR / 1000);
        board->set_pwm_ms(r_thr_ch, (double) DISARMED_THR / 1000);
    }
    s_arm_buff = control_sequence.s_arm;

    double elevator = (double) control_sequence.elevator/1000;
    board->set_pwm_ms(ele_ch, elevator);

    double aileron = (double) control_sequence.aileron/1000;
    board->set_pwm_ms(l_ail_ch, aileron);
    board->set_pwm_ms(r_ail_ch, aileron);
    // logger->trace("out {},{},{},{}", control_sequence.elevator, control_sequence.aileron, control_sequence.throttle, control_sequence.s_arm);
    return {};
}

IBUS::IBUS(
    std::string device,
    uint8_t ele_ch, 
    uint8_t ail_ch, 
    uint8_t thr_ch,
    uint8_t s_arm_ch,
    std::shared_ptr<spdlog::logger> input_log, std::chrono::time_point<std::chrono::steady_clock>* flog_starttime
): device(device), ele_ch(ele_ch), ail_ch(ail_ch), thr_ch(thr_ch), s_arm_ch(s_arm_ch) {
    logger = spdlog::get(LOGGER_IBUS_NAME);
    if (!logger) {
        logger = spdlog::default_logger(); // TODO proper fallback
    }

    ibus_config.device = this->device.c_str();
    context = ibus_init(&ibus_config);

    if (input_log) {
        this->input_log = input_log;
    }
    this->log_starttime = flog_starttime;

    logger->trace("PWM io module init done");
}

IBUS::~IBUS() {
    logger->trace("IBUS io deinit");
    ibus_deinit(context);
}


/*
Start pushing controls to q_ctrl
*/
void_expensive IBUS::start(Control* c_input_in, std::mutex* c_input_mut_in) {
    this->c_input = c_input_in;
    this->c_input_mut = c_input_mut_in;

    if (!context) {
        logger->error("Context was not set. Possible errors during construction");
        return std::unexpected(Error(InitError, "IBUS::start/IBUS::IBUS"));
    }

    if (ibus_set_channel_handler(context, handler, this)) {
        return {};
    } else {
        logger->error("Could not start ibus handler.");
        return std::unexpected(Error(InitError, "IBUS::start"));
    }
    
    return {};
}


/*
Is called each time there is a new IBUS packet. 
Important: pass `this` as a cookie in order to give the function access to the control queue

* Warning: performance critical!
*/
void IBUS::handler(ibus_channel_t*  channels_raw, void* cookie) {
    static auto* self = static_cast<IBUS*>(cookie);

    static double t;

    if ((!self) || (!self->c_input)) {
        if (self) {
            self->logger->error("Control Queue not found in IBUS io handler.");
        } else {
            spdlog::default_logger()->error("No object pointer was passed to the IBUS io handler. (Cannot access control queue)");
        }
        return;
    }
    
    if (channels_raw[self->ele_ch].value == 0 || channels_raw[self->ail_ch].value == 0 || channels_raw[self->thr_ch].value == 0) {
        self->logger->warn("Input value of 0 detected! IBUS may be dead");
        return;
    }

    if (self->input_log && self->log_starttime) {
        t = std::chrono::duration<double>(std::chrono::steady_clock::now() - *(self->log_starttime)).count();

        self->input_log->trace("{},{},{},{},{}",
            t,
            channels_raw[self->ele_ch].value, 
            channels_raw[self->ail_ch].value, 
            channels_raw[self->thr_ch].value, 
            channels_raw[self->s_arm_ch].value
        );
    }

    self->c_input_mut->lock();
    *(self->c_input) = Control(
        channels_raw[self->ele_ch].value, 
        channels_raw[self->ail_ch].value, 
        channels_raw[self->thr_ch].value, 
        channels_raw[self->s_arm_ch].value
    );
    self->c_input_mut->unlock();
}



IMU::IMU(
    std::string_view device,
    std::shared_ptr<spdlog::logger> imu_log, std::chrono::time_point<std::chrono::steady_clock>* flog_starttime,
    float bias_x,
    float bias_y,
    float bias_z
): device(device), bias_x(bias_x), bias_y(bias_y), bias_z(bias_z) {
    logger = spdlog::get(LOGGER_IMU_NAME);
    if (!logger) {
        spdlog::default_logger()->error("IMU Logger not found (IMU init)");
    }

    error* res = imu_init(&board, this->device.c_str(), IMU_ACC_RANGE_2G, IMU_GYRO_RANGE_250DPS);
    
    if(res != NULL) {
        if (!logger) {
            spdlog::default_logger()->error("IMU init failed!");
        } else {
            logger->error("IMU init failed!");
        }
    }
    if (imu_log) {
        this->imu_log = imu_log;
    }
    this->log_starttime = flog_starttime;
}

IMU::~IMU() {
    logger->trace("IMU io deinit");
    imu_destroy(board);
}

std::expected<RatesRead, ErrorType> IMU::read(bool incbias) {
    static imu_output io;
    static error* res;
    static float x,y,z;
    static double t;
    res = imu_read(board, &io);
    if(res != NULL) {
        logger->error("IMU read error: {}", res);
        return std::unexpected(ReadError);
    }

    // Remove bias
    x = (!incbias) ? io.gyro.x - bias_x : io.gyro.x; 
    y = (!incbias) ? io.gyro.y - bias_y : io.gyro.y;
    z = (!incbias) ? io.gyro.z - bias_z : io.gyro.z;

    if (this->imu_log && this->log_starttime) {
        t = std::chrono::duration<double>(std::chrono::steady_clock::now() - *(this->log_starttime)).count();

        this->imu_log->trace("{},{},{},{}",
            t,
            x,y,z
        );
    }

    return RatesRead(x,y,z);
}

/*
Calibrate the IMU (automatically set gyro bias).
* The drone should be completely stationary during this operation!

*/
void_expensive IMU::calibrate() {
    float bx,by,bz;
    for (int i = 0; i<=IMU_CAL_SAMPLESIZE; i++) {
        auto res = read(true);
        if (!res) {
            return std::unexpected(Error(res.error(), "Error reading IMU data"));
        }
        if (!res.value().valid) {
            // ! Change this behavior in the future
            logger->warn("Calibration: gyro reading is out of range (+-{} deg/s)", IMU_BOUNDS);
        }
        
        bx += res.value().x; 
        by += res.value().y; 
        bz += res.value().z; 
    }
    bias_x = bx / IMU_CAL_SAMPLESIZE;
    bias_y = by / IMU_CAL_SAMPLESIZE;
    bias_z = bz / IMU_CAL_SAMPLESIZE;
    return {};
}