#pragma once
#include <cstdint>
#include <boost/circular_buffer.hpp>


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
    inline Control(uint16_t ele, uint16_t ail, uint16_t thr): elevator(ele), aileron(ail), throttle(thr) {};
};

typedef boost::circular_buffer<Control> control_queue_t;

struct State {
    enum FlightState fstate;
    enum FlightMode mode;
};
