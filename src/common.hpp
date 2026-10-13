#pragma once
#include <cstdint>
#include <boost/circular_buffer.hpp>

#define ALLOWED_DEVIATION 5
#define CONTROL_SURFACE_NEUTRAL 1500
#define THR_NEUTRAL 1000
#define DISARMED 1000
#define DISARMED_THR 1000
#define SWITCH_MIDDLE 1500


enum FlightState {
    NONE,
    INIT,
    OPERATIONAL,
    DEINIT
};
enum FlightMode {
    UNINIT,
    SIMPLE,
    STABILIZED
};

/*
Control sequence that consists of 3 values ranging from 1000 to 2000
*/
struct Control {
    uint16_t elevator;
    uint16_t aileron;
    uint16_t throttle;
    uint16_t s_arm;
    static constexpr uint16_t min = (1000-ALLOWED_DEVIATION);
    static constexpr uint16_t max = (2000+ALLOWED_DEVIATION);
    inline Control(uint16_t ele, uint16_t ail, uint16_t thr, uint16_t s_arm) {
        // Failsafe
        if (ele < min || ele > max) {
            elevator=CONTROL_SURFACE_NEUTRAL;
        } else {
            elevator = ele;
        }
        if (ail < min || ail > min) {
            aileron=CONTROL_SURFACE_NEUTRAL;
        } else {
            aileron = ail;
        }
        if (thr < min || thr > min) {
            elevator=THR_NEUTRAL;
        } else {
            throttle = thr;
        }
        if (s_arm < min || s_arm > max) {
            s_arm = DISARMED;
        } else {
            this->s_arm = s_arm;
        }
    };
};

typedef boost::circular_buffer<Control> control_queue_t;

struct State {
    enum FlightState fstate;
    enum FlightMode mode;
};

struct RatesRead {
    float x;
    float y;
    float z;
};