#pragma once

#include <chrono>

namespace RhAL {

/**
 * Device unique id
 */
typedef int id_t;

/**
 * Device register address
 */
typedef size_t addr_t;

/**
 * Static all Device address space length.
 * Use to allocate memory inside Device.
 */
constexpr size_t AddrDevLen = 0xFF;

/**
 * Device model number
 */
typedef int type_t;

/**
 * Raw data
 */
typedef uint8_t data_t;

/**
 * Timestamp
 */
typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;

/**
 * Return the current date
 */
inline TimePoint getCurrentTimePoint()
{
    return std::chrono::steady_clock::now();
}

/**
 * A value associated with 
 * a timestamp
 */
template <typename T>
struct TimedValue {
    const TimePoint timestamp;
    const T value;

    //Simple initialization constructor
    TimedValue(const TimePoint& time, const T& val) :
        timestamp(time),
        value(val)
    {
    }
};

/**
 * Typedef for TimedValue
 */
typedef TimedValue<bool> TimedValueBool;
typedef TimedValue<int> TimedValueInt;
typedef TimedValue<float> TimedValueFloat;

}
