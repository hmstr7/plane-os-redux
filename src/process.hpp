#pragma once
#include "common.hpp"
#include "errors.hpp"
#include "helper.hpp"
#include "io.hpp"
#include <expected>
#include <memory>
#include <mutex>

enum ProcessState {
    NOT_RUNNING,
    RUNNING,
    FAILURE,
    STOPPED
};

struct Shared {
    /*
    private:
        std::mutex q_ctrl_lock{};
        control_queue_t q_ctrl;
    inline control_queue_t* get_q_ctrl() {   
    };
    */
    control_queue_t q_ctrl;
};

class Process {
    protected:
        Shared& shared_mem;
        bool initialized = false;
    public:
        inline Process(Shared& shared_mem): shared_mem(shared_mem) {};
        ProcessState state = ProcessState::NOT_RUNNING;
        virtual void_expensive init() = 0;
        virtual void operator()() = 0;
        virtual void stop() = 0;
};


class PWMProcess : public Process {
    private:
        std::unique_ptr<PWM> pwm_board;
    public:
        inline PWMProcess(Shared& shared_mem): Process(shared_mem) {};
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};

class IBUSProcess :public Process {
   private:
        std::unique_ptr<IBUS> ibus;
        ibus_context_t ctx;
        uart_lowlevel_config config;
      
        static void handler(ibus_channel_t* channels_raw, void* cookie);;
   public:
        inline IBUSProcess(Shared& shared_mem): Process(shared_mem) {};
        void_expensive init() override;
        void operator()() override;
        void stop() override;
};
