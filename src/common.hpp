#pragma once
#include <cstdint>
#include <boost/circular_buffer.hpp>

#define LOGDIR "logs/"
#define FLIGHTLOGPREFIX "flightlog"
#define FLIGHTLOGFILEXT ".csv"
#define FLIGHTLOGHEADER "t,ele,ail,thr,s_arm"

#define ALLOWED_DEVIATION 20
#define CONTROL_SURFACE_NEUTRAL 1500
#define THR_NEUTRAL 1000
#define DISARMED 1000
#define DISARMED_THR 1000
#define SWITCH_MIDDLE 1500
#define SWITCH_OFF 1000

/*
State of the vehicle
*/
enum FlightState {
    NONE, // unset / uninitialized
    INIT, // initialized
    OPERATIONAL, // currently operating
    DEINIT // deinitialization
};

/*
Flight Sub-Program
*/
enum Subprogram {
    IDLE, // Do nothing
    SIMPLE, // No stabilisation possible
    FULL // Able to switch between modes
};

/*
Control sequence that consists of 4 values ranging from 1000 to 2000
*/
struct Control {
    uint16_t elevator;
    uint16_t aileron;
    uint16_t throttle;
    uint16_t s_arm;
    static constexpr uint16_t min = (1000-ALLOWED_DEVIATION);
    static constexpr uint16_t max = (2000+ALLOWED_DEVIATION);
    inline Control(uint16_t ele, uint16_t ail, uint16_t thr, uint16_t s_arm_in) {
        // Failsafe
        if (ele < min || ele > max) {
            elevator=CONTROL_SURFACE_NEUTRAL;
        } else {
            elevator = ele;
        }
        if (ail < min || ail > max) {
            aileron=CONTROL_SURFACE_NEUTRAL;
        } else {
            aileron = ail;
        }
        if (thr < min || thr > max) {
            throttle=THR_NEUTRAL;
        } else {
            throttle = thr;
        }
        if (s_arm < min || s_arm > max) {
            s_arm = DISARMED;
        } else {
            this->s_arm = s_arm_in;
        }
    };
    inline bool operator==(Control& other) {
        return ((this->elevator==other.elevator) && (this->aileron==other.aileron) && (this->throttle==other.throttle) && (this->s_arm==other.s_arm));
    };
};
#define CONTROL_NEUTRAL Control(CONTROL_SURFACE_NEUTRAL,CONTROL_SURFACE_NEUTRAL,THR_NEUTRAL,SWITCH_OFF)
/*
Angular velocity reads produced by IMU
*/
struct RatesRead {
    float x;
    float y;
    float z;
};

typedef boost::circular_buffer<Control> control_queue_t; // Control queue (ex. q_input or q_output) is a ring buffer of Control objects, the new ones being appended to the end while the oldest ones are being read and deleted from the front.

struct State {
    enum FlightState fstate;
    enum Subprogram prog;
};

