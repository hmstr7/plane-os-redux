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
        logger->info("Initialized successfully");
        return {};
    } else {
        logger->warn("Already initialized");
        return {};
    }
}

void PWMProcess::operator()() {
    while(state == ProcessState::RUNNING) {
        Control current_control = shared_mem.q_ctrl.front();
        void_cheap exec_res = pwm_board->execute(current_control);
        if (!exec_res) {
            logger->error("Failed to execute control sequence: {}", ErrorTypeToStr.at(exec_res.error()));
            state = ProcessState::FAILURE;
        }
    }
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
        
        ibus = std::make_unique<IBUS>(
            device,
            ele_ch.value(),
            ail_ch.value(),
            thr_ch.value()
        );
        logger->info("Initialized successfully");
        return {};
    } else {
        // Do nothing
        return {};
    }
}

void IBUSProcess::operator()() {
    // TODO: error handling
    void_expensive start_res = this->ibus->start(&(shared_mem.q_ctrl));
    if (!start_res) {
        logger->error("Could not start handling IBUS packets: ", start_res.error());
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

