#include "io.hpp"
#include <cstdint>
#include "PiPCA9685/PCA9685.h"
#include <string>
#include <sys/types.h>
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
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
    board = std::make_unique<PiPCA9685::PCA9685>(device);
    board->set_pwm_freq(PWM_FREQ);
}

/*
Executes given control sequence.
Warning: performance critical!
*/
void_cheap PWM::execute(Control control_sequence) {
    double throttle = (double) control_sequence.throttle/1000;
    board->set_pwm_ms(l_thr_ch, throttle);
    board->set_pwm_ms(r_thr_ch, throttle);

    double elevator = (double) control_sequence.elevator/1000;
    board->set_pwm_ms(ele_ch, elevator);

    double aileron = (double) control_sequence.aileron/1000;
    board->set_pwm_ms(l_ail_ch, aileron);
    board->set_pwm_ms(r_ail_ch, -aileron);
}

IBUS::IBUS(
    std::string device,
    uint8_t ele_ch, 
    uint8_t ail_ch, 
    uint8_t thr_ch
): device(device), ele_ch(ele_ch), ail_ch(ail_ch), thr_ch(thr_ch) {
    ibus_config.device = this->device.c_str();
    context = ibus_init(&ibus_config);
}

IBUS::~IBUS() {
    ibus_deinit(context);
}


/*
Start pushing controls to q_ctrl
*/
void_expensive IBUS::start(control_queue_t* q_ctrl) {
    this->q_ctrl = q_ctrl;
    ibus_set_channel_handler(context, handler, this);
}

void IBUS::handler(ibus_channel_t*  channels_raw, void* cookie) {
    auto* self = static_cast<IBUS*>(cookie);

    self->q_ctrl->push(Control(channels_raw[self->ele_ch].value,channels_raw[self->ail_ch].value,channels_raw[self->thr_ch].value));
}