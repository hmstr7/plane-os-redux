#include "process.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "io.hpp"
#include <cstdint>
#include <expected>
#include <memory>
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <string>


PWMProcess::PWMProcess(Shared& shared_mem) : Process(shared_mem) {
    class_logger_name = LOGGER_PWM_NAME;
    INIT_CLASS_LOGGER
}

void_expensive PWMProcess::init() {
    logger->trace("Init"); 
    if (!initialized) {
        // getting values from config
        const auto config = get_hardware_config();
        if (!config) {
            logger->error("Failed to load config: ", config.error());
            return std::unexpected(config.error());
        }
        std::expected<std::string, Error> config_device = get_from_json<std::string>(config.value(), {"Output","PWM","device"}, "PWMProcess::init");
        if (!config_device) {
            logger->error("Failed to load device: ", config_device.error());
            return std::unexpected(config_device.error());
        }
        std::expected<int, Error> config_ele_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","ele"}, "PWMProcess::init");
        if (!config_ele_ch ) {
            logger->error("Failed to load ele_ch: ", config_ele_ch.error());
            return std::unexpected(config_ele_ch.error());
        }
        std::expected<int, Error> config_l_ail_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","l_ail"}, "PWMProcess::init");
        if (!config_l_ail_ch) {
            logger->error("Failed to load l_ail_ch: ", config_l_ail_ch.error());
            return std::unexpected(config_l_ail_ch.error());
        }
        std::expected<int, Error> config_r_ail_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","r_ail"}, "PWMProcess::init");
        if (!config_r_ail_ch) {
            logger->error("Failed to load r_ail_ch: ", config_r_ail_ch.error());
            return std::unexpected(config_r_ail_ch.error());
        }
        std::expected<int, Error> config_l_thr_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","l_thr"}, "PWMProcess::init");
        if (!config_l_thr_ch) {
            logger->error("Failed to load l_thr_ch: ", config_l_thr_ch.error());
            return std::unexpected(config_l_thr_ch.error());
        }
        std::expected<int, Error> config_r_thr_ch = get_from_json<int>(config.value(), {"Output","PWM","mapping","r_thr"}, "PWMProcess::init");
        if (!config_r_thr_ch) {
            logger->error("Failed to load r_thr_ch: ", config_r_thr_ch.error());
            return std::unexpected(config_r_thr_ch.error());
        }
        
        // conversion
        std::string device = config_device.value();
        auto ele_ch = to_uint8(config_ele_ch.value());
        if (!ele_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(ele_ch.error(), "PWMProcess::init"));
        }
        auto l_ail_ch = to_uint8(config_l_ail_ch.value());
        if (!l_ail_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(l_ail_ch.error(), "PWMProcess::init"));
        }
        auto r_ail_ch = to_uint8(config_r_ail_ch.value());
        if (!r_ail_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(r_ail_ch.error(), "PWMProcess::init"));
        }
        auto l_thr_ch = to_uint8(config_l_thr_ch.value());
        if (!l_thr_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(l_thr_ch.error(), "PWMProcess::init"));
        }
        auto r_thr_ch = to_uint8(config_r_thr_ch.value());
        if (!r_thr_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(r_thr_ch.error(), "PWMProcess::init"));
        }

        pwm_board = std::make_unique<PWM>(
            device,
            ele_ch.value(),
            l_ail_ch.value(),
            r_ail_ch.value(),
            l_thr_ch.value(), 
            r_thr_ch.value()
        );
        state = ProcessState::READY;
        logger->info("Initialized successfully");
        return {};
    } else {
        logger->warn("Already initialized");
        return {};
    }
}

void PWMProcess::operator()() {
    if (state == ProcessState::READY) {
        state = ProcessState::RUNNING;
    } else {
        logger->error("Run process.init() before starting a thread!");
        return;
    }
    while(state == ProcessState::RUNNING) {
        Control current_control = shared_mem.q_output.front();
        void_cheap exec_res = pwm_board->execute(current_control);
        if (!exec_res) {
            logger->error("Failed to execute control sequence: {}", ErrorTypeToStr.at(exec_res.error()));
            state = ProcessState::FAILURE;
        }
        shared_mem.q_output.pop_front();
    }
    if (!this->shared_mem.q_output.empty()) {
        // ? this behaviour probably needs to be changed later
        Control current_control = CONTROL_NEUTRAL;
        void_cheap exec_res = pwm_board->execute(current_control);
    }
    logger->trace("PWM thread stopped.");
}

void PWMProcess::stop() {
    if (state == STOPPED) {
        logger->warn("Process already stopped");
        return;
    }
    logger->trace("Stopping process");
    state = ProcessState::STOPPED;
}


IBUSProcess::IBUSProcess(Shared& shared_mem) : Process(shared_mem) {
    class_logger_name = LOGGER_IBUS_NAME;
    INIT_CLASS_LOGGER
}

void_expensive IBUSProcess::init() {
    logger->trace("Init"); 
    if (!initialized) {
        // getting values from config
        const auto config = get_hardware_config();
        if (!config) {
            logger->error("Failed to load config: ", config.error());
            return std::unexpected(config.error());
        }
        std::expected<std::string, Error> config_device = get_from_json<std::string>(config.value(), {"Output","IBUS","device"}, "PWMProcess::init");
        if (!config_device) {
            logger->error("Failed to load device: ", config_device.error());
            return std::unexpected(config_device.error());
        }
        std::expected<int, Error> config_ele_ch = get_from_json<int>(config.value(), {"Output","IBUS","mapping","ele"}, "PWMProcess::init");
        if (!config_ele_ch ) {
            logger->error("Failed to load ele_ch: ", config_ele_ch.error());
            return std::unexpected(config_ele_ch.error());
        }
        std::expected<int, Error> config_ail_ch = get_from_json<int>(config.value(), {"Output","IBUS","mapping","ail"}, "PWMProcess::init");
        if (!config_ail_ch) {
            logger->error("Failed to load ail_ch: ", config_ail_ch.error());
            return std::unexpected(config_ail_ch.error());
        }
        std::expected<int, Error> config_thr_ch = get_from_json<int>(config.value(), {"Output","IBUS","mapping","thr"}, "PWMProcess::init");
        if (!config_thr_ch) {
            logger->error("Failed to load thr_ch: ", config_thr_ch.error());
            return std::unexpected(config_thr_ch.error());
        }
        std::expected<int, Error> config_s_arm_ch = get_from_json<int>(config.value(), {"Output","IBUS","mapping","s_arm"}, "PWMProcess::init");
        if (!config_s_arm_ch) {
            logger->error("Failed to load s_arm_ch: ", config_s_arm_ch.error());
            return std::unexpected(config_s_arm_ch.error());
        }
        
        
        // conversion
        std::string device = config_device.value();
        auto ele_ch = to_uint8(config_ele_ch.value());
        if (!ele_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(ele_ch.error(), "PWMProcess::init"));
        }
        auto ail_ch = to_uint8(config_ail_ch.value());
        if (!ail_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(ail_ch.error(), "PWMProcess::init"));
        }
        auto thr_ch = to_uint8(config_thr_ch.value());
        if (!thr_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(thr_ch.error(), "PWMProcess::init"));
        }
        auto s_arm_ch = to_uint8(config_s_arm_ch.value());
        if (!s_arm_ch) {
            logger->error("Conversion error");
            return std::unexpected(Error(s_arm_ch.error(), "PWMProcess::init"));
        }
        
        ibus = std::make_unique<IBUS>(
            device,
            ele_ch.value(),
            ail_ch.value(),
            thr_ch.value(),
            s_arm_ch.value()
        );

        state = ProcessState::READY;
        logger->info("Initialized successfully");
        return {};
    } else {
        // Do nothing
        return {};
    }
}

void IBUSProcess::operator()() {
    if (state == ProcessState::READY) {
        state = ProcessState::RUNNING;
    } else {
        logger->error("Run process.init() before starting a thread!");
        return;
    }
    void_expensive start_res = this->ibus->start(&(shared_mem.q_input));
    if (!start_res) {
        logger->error("Could not start handling IBUS packets: ", start_res.error());
        state = ProcessState::FAILURE;
        return;
    }
}

void IBUSProcess::stop() {
    if (state == STOPPED) {
        logger->warn("Process already stopped");
        return;
    }
    logger->trace("Stopping process");
    this->ibus.reset();
    this->ibus = nullptr;
    this->state = ProcessState::STOPPED;
}



Middleman::Middleman(Shared& shared_mem): Process(shared_mem) {
    class_logger_name = LOGGER_MIDDLEMAN_NAME;
    INIT_CLASS_LOGGER
}

void_expensive Middleman::init() {
    this->logger->trace("Init");
    switch (this->shared_mem.middleman_mode) {
        case STANDBY:
            this->logger->info("The middleman is in standby.");
        case PASSTHROUGH:
            this->logger->warn("The middleman is in the passthrough mode at init.");
        case RATE_STAB:
            this->logger->warn("The middleman is in the rate stabilisation mode at init.");
        default:
            this->logger->error("The middleman is set to an unimplemented or invalid mode at init, aborting");
            this->state = FAILURE;
            return std::unexpected<Error>(Error(ErrorType::InitError,"Unknown middleman mode at init."));
    } 
    this->state = READY;
}

void Middleman::operator()() {
    this->state = RUNNING;
    while (this->state == RUNNING) {
        if (this->shared_mem.mmode_user_allowed) {
            // TODO Real time mode switching should be done when the stabilisation is finished
        }
        switch (this->shared_mem.middleman_mode) {
            case STANDBY:
                if (lastm == PASSTHROUGH || lastm == RATE_STAB) {
                    // Output has to be reset to a neutral position
                    this->logger->info("Standby");
                    this->shared_mem.q_output.push_back(CONTROL_NEUTRAL);
                } else if (lastm == STANDBY) {} else {
                    this->logger->info("Standby");
                }
                // do nothing
            case PASSTHROUGH: {
                if (lastm != PASSTHROUGH) {
                    this->logger->info("Passthrough mode");
                }
                Control sel_ctrl = this->shared_mem.q_input.front();
                if (sel_ctrl != ctrl_buff) {
                    this->shared_mem.q_output.push_back(sel_ctrl); 
                    this->shared_mem.q_input.pop_front();
                    ctrl_buff = sel_ctrl;
                }
            }
            case RATE_STAB:
                if (lastm != RATE_STAB) {
                        this->logger->info("Rate stabilisation mode");
                } 
                // TODO: not implemented
            default:
                this->logger->error("The middleman is set to an unimplemented or invalid mode, falling back to standby.");
                this->shared_mem.middleman_mode = STANDBY;
                // ? This may be bad
        } 
    }
}

void Middleman::stop() {
    if (this->state == STOPPED) {
        logger->warn("Process already stopped");
        return;
    }
    logger->trace("Stopping process");
    if (lastm == PASSTHROUGH || lastm == RATE_STAB) {
        logger->trace("Sending neutral control object");
        this->shared_mem.q_output.push_back(CONTROL_NEUTRAL);
    }
    this->state = STOPPED;
}



