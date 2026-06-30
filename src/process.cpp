#include "process.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "io.hpp"
#include <cstdint>
#include <expected>
#include <memory>
#include <string>

void_expensive PWMProcess::init() {
    if (!initialized) {
        // getting values from config
        const auto config = get_hardware_config();
        if (!config) {
            return std::unexpected(config.error());
        }
        std::expected<std::string, Error> config_device = get_from_json<std::string>(config.value(), {"Output","PWM","device"}, "PWMProcess::init");
        if (!config_device) {
            return std::unexpected(config_device.error());
        }
        std::expected<int, Error> config_ele_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","ele"}, "PWMProcess::init");
        if (!config_ele_ch ) {
            return std::unexpected(config_ele_ch.error());
        }
        std::expected<int, Error> config_l_ail_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","l_ail"}, "PWMProcess::init");
        if (!config_l_ail_ch) {
            return std::unexpected(config_l_ail_ch.error());
        }
        std::expected<int, Error> config_r_ail_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","r_ail"}, "PWMProcess::init");
        if (!config_r_ail_ch) {
            return std::unexpected(config_r_ail_ch.error());
        }
        std::expected<int, Error> config_l_thr_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","l_thr"}, "PWMProcess::init");
        if (!config_l_thr_ch) {
            return std::unexpected(config_l_thr_ch.error());
        }
        std::expected<int, Error> config_r_thr_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","r_thr"}, "PWMProcess::init");
        if (!config_r_thr_ch) {
            return std::unexpected(config_r_thr_ch.error());
        }
        
        // conversion
        std::string device = config_device.value();
        auto ele_ch = to_uint8(config_ele_ch.value());
        if (!ele_ch) {
            return std::unexpected(Error(ele_ch.error(), "PWMProcess::init"));
        }
        auto l_ail_ch = to_uint8(config_l_ail_ch.value());
        if (!l_ail_ch) {
            return std::unexpected(Error(l_ail_ch.error(), "PWMProcess::init"));
        }
        auto r_ail_ch = to_uint8(config_r_ail_ch.value());
        if (!r_ail_ch) {
            return std::unexpected(Error(r_ail_ch.error(), "PWMProcess::init"));
        }
        auto l_thr_ch = to_uint8(config_l_thr_ch.value());
        if (!l_thr_ch) {
            return std::unexpected(Error(l_thr_ch.error(), "PWMProcess::init"));
        }
        auto r_thr_ch = to_uint8(config_r_thr_ch.value());
        if (!r_thr_ch) {
            return std::unexpected(Error(ele_ch.error(), "PWMProcess::init"));
        }

        pwm_board = std::make_unique<PWM>(
            device,
            ele_ch.value(),
            l_ail_ch.value(),
            r_ail_ch.value(),
            l_thr_ch.value(), 
            r_thr_ch.value()
        );
        return {};
    } else {
        // Do nothing
        return {};
    }
}

void PWMProcess::operator()() {
    while(state == ProcessState::RUNNING) {
        Control current_control = shared_mem.q_ctrl.front();
        void_cheap exec_res = pwm_board->execute(current_control);
        if (!exec_res) {
            // Handle job failure here
            state = ProcessState::FAILURE;
        }
    }
}

void PWMProcess::stop() {
    state = ProcessState::STOPPED;
}


void_expensive IBUSProcess::init() {
        if (!initialized) {
            // getting values from config
            const auto config = get_hardware_config();
            if (!config) {
                return std::unexpected(config.error());
            }
            std::expected<std::string, Error> config_device = get_from_json<std::string>(config.value(), {"Output","IBUS","device"}, "PWMProcess::init");
            if (!config_device) {
                return std::unexpected(config_device.error());
            }
            std::expected<int, Error> config_ele_ch = get_from_json<int>(config.value(), {"Output","IBUS","mapping","ele"}, "PWMProcess::init");
            if (!config_ele_ch ) {
                return std::unexpected(config_ele_ch.error());
            }
            std::expected<int, Error> config_ail_ch = get_from_json<int>(config.value(), {"Output","IBUS","mapping","ail"}, "PWMProcess::init");
            if (!config_ail_ch) {
                return std::unexpected(config_ail_ch.error());
            }
            std::expected<int, Error> config_thr_ch = get_from_json<int>(config.value(), {"Output","IBUS","mapping","thr"}, "PWMProcess::init");
            if (!config_thr_ch) {
                return std::unexpected(config_thr_ch.error());
            }
            
            
            // conversion
            std::string device = config_device.value();
            auto ele_ch = to_uint8(config_ele_ch.value());
            if (!ele_ch) {
                return std::unexpected(Error(ele_ch.error(), "PWMProcess::init"));
            }
            auto ail_ch = to_uint8(config_ail_ch.value());
            if (!ail_ch) {
                return std::unexpected(Error(ail_ch.error(), "PWMProcess::init"));
            }
            auto thr_ch = to_uint8(config_thr_ch.value());
            if (!thr_ch) {
                return std::unexpected(Error(thr_ch.error(), "PWMProcess::init"));
            }
            
            ibus = std::make_unique<IBUS>(
                device,
                ele_ch.value(),
                ail_ch.value(),
                thr_ch.value()
            );
            return {};
        } else {
            // Do nothing
            return {};
        }
}

void IBUSProcess::operator()() {
    // TODO: error handling
    this->ibus->start(&(shared_mem.q_ctrl));
}

void IBUSProcess::stop() {
    this->ibus.reset();
    this->ibus = nullptr;
}

