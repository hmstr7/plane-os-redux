#pragma once
#include <expected>
#include <queue>
#include <cstdint>
#include <string_view>
#include "PiPCA9685/PCA9685.h"
#include "errors.hpp"
#include "common.hpp"
extern "C" {
    #include "ibus/ibus.h"
}


#define PWM_FREQ 50.0

class PWM {
    private:
        // config

        const std::string device;
        const uint8_t ele_ch;
        const uint8_t l_ail_ch;
        const uint8_t r_ail_ch;
        const uint8_t l_thr_ch;
        const uint8_t r_thr_ch;

        std::unique_ptr<PiPCA9685::PCA9685> board;
    public:
        PWM(
            std::string device,
            uint8_t ele_ch,
            uint8_t l_ail_ch,
            uint8_t r_ail_ch,
            uint8_t l_thr_ch,
            uint8_t r_thr_ch
        );
        void_cheap execute(Control control_sequence);
};

class IBUS {
    private:
        bool initialized = false;

        ibus_context_t context;
        uart_lowlevel_config ibus_config;

        const std::string device;
        const uint8_t ele_ch;
        const uint8_t ail_ch;
        const uint8_t thr_ch;
        control_queue_t* q_ctrl = nullptr;

        static void handler(ibus_channel_t* channels_raw, void* cookie);
    public:
        IBUS(
            std::string device,
            uint8_t ele_ch, 
            uint8_t ail_ch, 
            uint8_t thr_ch
        );
        ~IBUS();
        void_expensive start(control_queue_t* q_ctrl);
};