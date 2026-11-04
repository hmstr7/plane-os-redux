#include "process.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "helper.tpp"
#include "io.hpp"
#include <chrono>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <iomanip>
#include <memory>
#include "spdlog/common.h"
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <sstream>
#include <string>
#include <iostream>


PWMProcess::PWMProcess(Shared& shared_mem) : Process(shared_mem) {
    class_logger_name = LOGGER_PWM_NAME;
    INIT_CLASS_LOGGER
}

void_expensive PWMProcess::init() {
    if (this->state == NOT_READY) {
        logger->trace("Init"); 
        // getting values from config
        const auto config = get_hardware_config();
        if (!config) {
            logger->error("Failed to load config: ", config.error());
            return std::unexpected(config.error());
        }


        JSONGET(config, config_device, std::string, "IO","PWM","device")
        JSONGET(config, config_ele_ch, int,"IO","PWM","mapping","ele")
        JSONGET(config, config_l_ail_ch, int,"IO","PWM","mapping","l_ail")
        JSONGET(config, config_r_ail_ch, int,"IO","PWM","mapping","r_ail")
        JSONGET(config, config_l_thr_ch, int,"IO","PWM","mapping","l_thr")
        JSONGET(config, config_r_thr_ch, int,"IO","PWM","mapping","r_thr")
        
        // conversion
        std::string device = config_device.value();
        TO_UINT8(config_ele_ch, ele_ch)
        TO_UINT8(config_l_ail_ch, l_ail_ch)
        TO_UINT8(config_r_ail_ch, r_ail_ch)
        TO_UINT8(config_l_thr_ch, l_thr_ch)
        TO_UINT8(config_r_thr_ch, r_thr_ch)

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
        logger->warn("PWM process is already initialized");
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
        shared_mem.c_output_mut.lock();
        Control current_control = shared_mem.c_output;
        shared_mem.c_output_mut.unlock();
        void_cheap exec_res = pwm_board->execute(current_control);
        if (!exec_res) {
            logger->error("Failed to execute control sequence: {}", ErrorTypeToStr.at(exec_res.error()));
            state = ProcessState::FAILED;
        }
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
    if (this->state == NOT_READY) {
        logger->trace("Init"); 
        // getting values from config
        const auto config = get_hardware_config();
        if (!config) {
            logger->error("Failed to load config: ", config.error());
            return std::unexpected(config.error());
        }
        
        std::initializer_list<std::string_view> path = {};
        JSONGET(config, config_device, std::string,"IO","IBUS","device")
        JSONGET(config, config_ele_ch, int,"IO","IBUS","mapping","ele")
        JSONGET(config, config_ail_ch, int,"IO","IBUS","mapping","ail")
        JSONGET(config, config_thr_ch, int,"IO","IBUS","mapping","thr")
        JSONGET(config, config_s_arm_ch, int,"IO","IBUS","mapping","s_arm")
        
        // conversion
        std::string device = config_device.value();
        TO_UINT8(config_ele_ch, ele_ch)
        TO_UINT8(config_ail_ch, ail_ch)
        TO_UINT8(config_thr_ch, thr_ch)
        TO_UINT8(config_s_arm_ch, s_arm_ch)

    
        if (shared_mem.f_logflightdata) {
            logger->trace("Creating input flightlog file...");
            std::ostringstream lfilename;
            lfilename << LOGDIR;
            lfilename << FLIGHTLOGPREFIX << "-";
            // add timestamp
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm localTime = *std::localtime(&time);
            lfilename << std::put_time(&(localTime), "%Y-%m-%d_%H-%M-%S");
            
            lfilename << "-input" << FLIGHTLOGFILEXT;

            this->input_log = spdlog::basic_logger_mt<spdlog::async_factory>("input_logger", lfilename.str()); 
            if (!this->input_log) {
                logger->error("Could not create log file {}", lfilename.str());
            } else {
                this->input_log->set_level(spdlog::level::trace);
                this->input_log->set_pattern("%v"); // so it is a valid csv
                this->input_log->trace(FLIGHTLOGHEADER_INPUT);
            }
            logger->trace("Done creating input flightlog file.");
        }

        ibus = std::make_unique<IBUS>(
            device,
            ele_ch.value(),
            ail_ch.value(),
            thr_ch.value(),
            s_arm_ch.value(),
            input_log,
            &(shared_mem.flightlog_starttime)
        );

        state = ProcessState::READY;
        logger->info("Initialized successfully");
        return {};
    } else {
        logger->warn("IBUS process is already initialized");
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
    void_expensive start_res = this->ibus->start(&(shared_mem.c_input), &(shared_mem.c_input_mut));
    if (!start_res) {
        logger->error("Could not start handling IBUS packets: ", start_res.error());
        state = ProcessState::FAILED;
        return;
    }
}

void IBUSProcess::stop() {
    if (state == STOPPED) {
        logger->warn("Process already stopped");
        return;
    }
    logger->trace("Stopping process");
    this->state = ProcessState::STOPPED;
    this->ibus.reset();
    this->ibus = nullptr;
}



Middleman::Middleman(Shared& shared_mem): Process(shared_mem) {
    class_logger_name = LOGGER_MIDDLEMAN_NAME;
    INIT_CLASS_LOGGER
}

void_expensive Middleman::init() {
    if (this->state == NOT_READY) {
        this->logger->trace("Init");
        switch (this->shared_mem.middleman_mode) {
            case STANDBY:
                this->logger->info("The middleman is in standby.");
                break;
            case PASSTHROUGH:
                this->logger->warn("The middleman is in the passthrough mode at init.");
                break;
            case RATE_STAB:
                this->logger->warn("The middleman is in the rate stabilisation mode at init.");
                break;
            default:
                this->logger->error("The middleman is set to an unimplemented or invalid mode at init, aborting");
                this->state = FAILED;
                return std::unexpected<Error>(Error(ErrorType::InitError,"Unknown middleman mode at init."));
        } 
        this->state = READY;
        this->logger->info("Initialized successfully");
        return {};
    } else {
        this->logger->warn("Middleman process is already initialized");
        return {};
    }
    
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

                    this->shared_mem.c_output_mut.lock();
                    this->shared_mem.c_output = CONTROL_NEUTRAL;
                    this->shared_mem.c_output_mut.unlock();
                } else if (lastm == STANDBY) {} else {
                    this->logger->info("Standby");
                }
                // do nothing
                lastm = STANDBY;
                break;
            case PASSTHROUGH: {
                if (lastm != PASSTHROUGH) {
                    this->logger->info("Passthrough mode");
                }
                
                this->shared_mem.c_input_mut.lock();
                if (this->shared_mem.c_input != ctrl_buff) {
                    this->shared_mem.c_output = this->shared_mem.c_input; 
                    ctrl_buff = this->shared_mem.c_input;
                }
                this->shared_mem.c_input_mut.unlock();
                lastm = PASSTHROUGH;
                break;
            }
            case RATE_STAB:
                if (lastm != RATE_STAB) {
                        this->logger->info("Rate stabilisation mode");
                } 
                // TODO: not implemented
                lastm = RATE_STAB;
                break;
            default:
                this->logger->error("The middleman is set to an unimplemented or invalid mode, falling back to standby.");
                this->shared_mem.middleman_mode = STANDBY;
                // ? This may be bad
                break;
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
        this->shared_mem.c_output_mut.lock();
        this->shared_mem.c_output = CONTROL_NEUTRAL;
        this->shared_mem.c_output_mut.unlock();
    }
    this->state = STOPPED;
}

IMUProcess::IMUProcess(Shared& shared_mem): Process(shared_mem) {
    class_logger_name = LOGGER_IMU_NAME;
    INIT_CLASS_LOGGER
}

void_expensive IMUProcess::init() {
    if (this->state == NOT_READY) {
        logger->trace("Init");

        auto hconfig = get_hardware_config();
        if (!hconfig) {
            return std::unexpected(hconfig.error());
        }
        
        auto device = get_from_json<std::string>(hconfig.value(), {"IO","IMU","device"});
        if (!device) {
            return std::unexpected(device.error());
        }
        
        auto autoconf = get_auto_config();
        if (!autoconf) {
            return std::unexpected(autoconf.error()); // ! problematic?
        }
        
        bool fail = false;

        JSONGET_BIAS(autoconf, bias_x, float, "imu_gyro_bias_x")
        JSONGET_BIAS(autoconf, bias_y, float, "imu_gyro_bias_y")
        JSONGET_BIAS(autoconf, bias_z, float, "imu_gyro_bias_z")

        if (shared_mem.f_logflightdata) {
            std::ostringstream lfilename;
            lfilename << LOGDIR;
            lfilename << FLIGHTLOGPREFIX << "-";
            // add timestamp
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm localTime = *std::localtime(&time);
            lfilename << std::put_time(&(localTime), "%Y-%m-%d_%H-%M-%S");
            
            lfilename << "-imu" << FLIGHTLOGFILEXT;

            this->imu_log = spdlog::basic_logger_mt<spdlog::async_factory>("imu_logger", lfilename.str()); // ! no error handling 
            this->imu_log->set_level(spdlog::level::trace);
            this->imu_log->set_pattern("%v"); // so it is a valid csv
            this->imu_log->trace(FLIGHTLOGHEADER_IMU);
        }

        if (fail) {
            this->imu = std::make_unique<IMU>(device.value(),imu_log, &(shared_mem.flightlog_starttime));
        } else {
            this->imu = std::make_unique<IMU>(device.value(),imu_log, &(shared_mem.flightlog_starttime),bias_x.value(),bias_y.value(),bias_z.value());
        }

        // TODO: UI
        // Calibration prompt 
        if (shared_mem.f_prompt_calibrate) {
            bool ans_valid = false;
            while (!ans_valid) {
                std::string ans;
                std::cout << "\n(calib) Calibrate? [y/n] ";
                std::cin >> ans;
                if (ans == "y" || ans == "Y" || ans == "Yes" || ans == "yes") {
                    ans_valid = true;
                    shared_mem.f_should_calibrate = true;
                } else if (ans == "n" || ans == "N" || ans == "No" || ans == "no") {
                    ans_valid = true;
                    shared_mem.f_should_calibrate = false;
                } else {
                    std::cout << "\n Please enter either y or n. \n";
                }
            }
        }
        if (shared_mem.f_should_calibrate) {
            logger->trace("Begin calibrate");
            std::cout << "\n(calib) Position the drone so that it is completely stationary during calibration. \nWhen ready, press [Enter] ";
            std::cin;
            std::cout << "\n(calib) Calibration in progress...";
            auto res = imu->calibrate();
            if (!res) {
                logger->error("Calibration failed: {}",res.error().to_string());
                std::cout << "\n(calib) Calibration failed.\n";

                if (POLICY_CALIBFAILEDSTOPSPROG) {
                    return std::unexpected(Error(InitError, "Calibration process failed"));
                }
            } else {
                logger->info("Calibrated successfully");
                std::cout << "\n(calib) Calibrated successfully.\n";

                bool fail = false;
                SAVEBIAS(autoconf,biasxres,"imu_gyro_bias_x", 0);
                SAVEBIAS(autoconf,biasyres,"imu_gyro_bias_y", 1);
                SAVEBIAS(autoconf,biaszres,"imu_gyro_bias_z", 2);
                if (fail) {
                    logger->error("Failed to save calibration data: {}", biasxres.error().to_string());
                }

            }
        }

        this->state = READY;
        this->logger->info("Initialized successfully");
    } else {
        this->logger->warn("IMU process is already initialized");
        return {};
    }
    return {};
}

void IMUProcess::operator()() {
    this->state = RUNNING;
    while (this->state == RUNNING) {
        
        auto res = imu->read();
        if (!res) {
            logger->error("A ({}) has occured while trying to read IMU data", ErrorTypeToStr.at(res.error()));
            continue;
        }
        if (!res.value().valid) {
            logger->warn("Received gyro readings are invalid. {}", POLICY_DROPINVALIDRATES ? "Dropping." : "");
            if (POLICY_DROPINVALIDRATES) {
                continue;
            }
        }
        this->shared_mem.c_imu_mut.lock();
        this->shared_mem.c_imu = res.value();
        this->shared_mem.c_imu_mut.unlock();
    }
}

void IMUProcess::stop() {
    if (state == STOPPED) {
        logger->warn("Process already stopped");
        return;
    }
    logger->trace("Stopping process");
    this->state = STOPPED;
    this->imu.reset();
    this->imu = nullptr;
}
