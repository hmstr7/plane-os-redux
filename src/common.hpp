#pragma once
#include <cstdint>

#define LOGDIR "../logs/"
#define FLIGHTLOGPREFIX "flightlog"
#define FLIGHTLOGFILEXT ".csv"
#define FLIGHTLOGHEADER_INPUT "t,ele,ail,thr,s_arm"
#define FLIGHTLOGHEADER_IMU "t,x,y,z"

#define ALLOWED_DEVIATION 20
#define CONTROL_SURFACE_NEUTRAL 1500
#define THR_NEUTRAL 1000
#define DISARMED 1000
#define DISARMED_THR 1000
#define SWITCH_MIDDLE 1500
#define SWITCH_OFF 1000
#define IMU_BOUNDS 255 /** An IMU gyro reading with a value outside +- this value will be considered invalid. (currently set to 250+5 because of imprecisions)*/

#define POLICY_DROPINVALIDRATES false
#define POLICY_CALIBFAILEDSTOPSPROG false

/**
 * Represents global state of the drone
 */
enum FlightState {
    NONE /** unset / uninitialized */
    ,INIT /** initialized */
    ,OPERATIONAL /** currently operating */
    ,DEINIT /** deinitialization */
};

/**
 * Flight Sub-Program
 */
enum Subprogram {
    IDLE /** Do nothing */
    ,SIMPLE /** No stabilisation possible */
    ,FULL /** Able to switch between modes */
};

/**
 * Control sequence
 * 
 * @details Consists of 4 values ranging from 1000 to 2000 (pulsewidth in us). 
 * If any of the values is past allowed limits, a corresponding default value is assigned defined in common.hpp
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

/**
 * Angular velocity reads produced by IMU
 */
struct RatesRead {
    bool valid = false;
    float dt;
    float x;
    float y;
    float z;
    inline RatesRead(float dt, float x, float y, float z): dt(dt), x(x), y(y), z(z) {
        this->valid = 
            !(dt < 0) &&
            !(x < -IMU_BOUNDS || IMU_BOUNDS > x) && 
            !(y < -IMU_BOUNDS || IMU_BOUNDS > y) && 
            !(z < -IMU_BOUNDS || IMU_BOUNDS > z);
    }
};

struct State {
    enum FlightState fstate;
    enum Subprogram prog;
};

