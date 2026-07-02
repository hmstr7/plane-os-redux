#pragma once
#include <cstdint>
#include <boost/circular_buffer.hpp>

#define ALLOWED_DEVIATION 5
#define CONTROL_SURFACE_NEUTRAL 1500
#define THR_NEUTRAL 1000

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
    inline Control(uint16_t ele, uint16_t ail, uint16_t thr) {
        // Failsafe
        if (ele < (1000-ALLOWED_DEVIATION) || ele > (2000+ALLOWED_DEVIATION)) {
            elevator=CONTROL_SURFACE_NEUTRAL;
        } else {
            elevator = ele;
        }
        if (ail < (1000-ALLOWED_DEVIATION) || ail > (2000+ALLOWED_DEVIATION)) {
            aileron=CONTROL_SURFACE_NEUTRAL;
        } else {
            aileron = ail;
        }
        if (thr < (1000-ALLOWED_DEVIATION) || thr > (2000+ALLOWED_DEVIATION)) {
            elevator=THR_NEUTRAL;
        } else {
            throttle = thr;
        }
    };
};

typedef boost::circular_buffer<Control> control_queue_t;

struct State {
    enum FlightState fstate;
    enum FlightMode mode;
};
