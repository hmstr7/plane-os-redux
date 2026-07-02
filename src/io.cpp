#include "io.hpp"
#include <cstdint>
#include "PiPCA9685/PCA9685.h"
#include "spdlog/spdlog.h"
#include <string>
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "process.hpp"
extern "C" {
    #include "ibus/ibus.h"
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
) : device(device), ele_ch(ele_ch), l_ail_ch(l_ail_ch), r_ail_ch(r_ail_ch), l_thr_ch(l_thr_ch), r_thr_ch(r_ail_ch) {
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
Warning: performance critical!
*/
void_cheap PWM::execute(Control& control_sequence) {
    double throttle = (double) control_sequence.throttle/1000;
    board->set_pwm_ms(l_thr_ch, throttle);
    board->set_pwm_ms(r_thr_ch, throttle);

    double elevator = (double) control_sequence.elevator/1000;
    board->set_pwm_ms(ele_ch, elevator);

    double l_aileron = (double) control_sequence.aileron/1000;
    double r_aileron = 3.0 - l_aileron;
    board->set_pwm_ms(l_ail_ch, l_aileron);
    board->set_pwm_ms(r_ail_ch, r_aileron);
    return {};
}

IBUS::IBUS(
    std::string device,
    uint8_t ele_ch, 
    uint8_t ail_ch, 
    uint8_t thr_ch
): device(device), ele_ch(ele_ch), ail_ch(ail_ch), thr_ch(thr_ch) {
    logger = spdlog::get(LOGGER_IBUS_NAME);
    if (!logger) {
        logger = spdlog::default_logger(); // TODO proper fallback
    }

    ibus_config.device = this->device.c_str();
    context = ibus_init(&ibus_config);
    logger->trace("PWM io module init done");
}

IBUS::~IBUS() {
    logger->trace("IBUS io deinit");
    ibus_deinit(context);
}


/*
Start pushing controls to q_ctrl
*/
void_expensive IBUS::start(control_queue_t* q_ctrl) {
    this->q_ctrl = q_ctrl;

    if (!context) {
        logger->error("Context was not set. Possible errors during construction");
        return std::unexpected(Error(InitError, "IBUS::start/IBUS::IBUS"));
    }

    ibus_set_channel_handler(context, handler, this);
    
    return {};
}


/*
Is called each time there is a new IBUS packet. 
Important: pass `this` as a cookie in order to give the function access to the control queue

Warning: performance critical!
*/
void IBUS::handler(ibus_channel_t*  channels_raw, void* cookie) {
    auto* self = static_cast<IBUS*>(cookie);

    if ((!self) || (!self->q_ctrl)) {
        if (self) {
            self->logger->error("Control Queue not found in IBUS io handler.");
        } else {
            spdlog::default_logger()->error("No object pointer was passed to the IBUS io handler. (Cannot access control queue)");
        }
        return;
    }

    self->q_ctrl->push_back(
        Control(
            channels_raw[self->ele_ch].value,
            channels_raw[self->ail_ch].value,
            channels_raw[self->thr_ch].value
        )
    );
}