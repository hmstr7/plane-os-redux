#include "helper.hpp"
#include "errors.hpp"
#include <cstdint>
#include <expected>
#include <fstream>
#include <initializer_list>
#include <string_view>

/**
 * @name Type conversions
 * @{
 */

/**
 * Convert an integer to unsigned 8 bit integer.
 * 
 * @param value must be a integer between 0 and 255
 */
std::expected<uint8_t, ErrorType> to_uint8(int value) {
    if (!((value>=0) && (value<=UINT8_MAX))) {
        return std::unexpected(ErrorType::OutOfRangeError);
    }

    return static_cast<uint8_t>(value);
}

/**
 * Convert an integer to unsigned 16 bit integer.
 * 
 * @param value must be a integer between 0 and 65535
 */
std::expected<uint16_t, ErrorType> to_uint16(int value) {
    if (!((value>=0) && (value<=UINT16_MAX))) {
        return std::unexpected(ErrorType::OutOfRangeError);
    }

    return static_cast<uint16_t>(value);
}

/**
 * Convert an integer to unsigned 32 bit integer.
 * 
 * @param value must be a integer between 0 and 4294967295
 */
std::expected<uint32_t, ErrorType> to_uint32(int value) {
    if (!((value>=0) && (value<=UINT32_MAX))) {
        return std::unexpected(ErrorType::OutOfRangeError);
    }

    return static_cast<uint32_t>(value);
}

/**
 * @}
 */