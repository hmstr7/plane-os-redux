#include "helper.hpp"
#include "errors.hpp"
#include <cstdint>
#include <expected>
#include <fstream>
#include <initializer_list>
#include <string_view>


/*
Gets con
*/
std::expected<nlohmann::json, Error> get_hardware_config() {
    std::ifstream f(CONFIG_PATH_HW);
    if (!f) {
        return std::unexpected(Error(ErrorType::FileError,"get_hardware_config"));
    }

    nlohmann::json config = nlohmann::json::parse(f,nullptr,false);
    if (config.is_discarded()) {
        return std::unexpected(Error(ErrorType::ParseError,"get_hardware_config"));
    }

    return config;
}

// get_from_json moved to tpp

// Type conversions //

std::expected<uint8_t, ErrorType> to_uint8(int value) {
    if (!((value>=0) && (value<=UINT8_MAX))) {
        return std::unexpected(ErrorType::OutOfRangeError);
    }

    return static_cast<uint8_t>(value);
}
std::expected<uint16_t, ErrorType> to_uint16(int value) {
    if (!((value>=0) && (value<=UINT16_MAX))) {
        return std::unexpected(ErrorType::OutOfRangeError);
    }

    return static_cast<uint16_t>(value);
}
std::expected<uint32_t, ErrorType> to_uint32(int value) {
    if (!((value>=0) && (value<=UINT32_MAX))) {
        return std::unexpected(ErrorType::OutOfRangeError);
    }

    return static_cast<uint32_t>(value);
}