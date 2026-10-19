#include "helper.hpp"
#include "errors.hpp"
#include <cstdint>
#include <expected>
#include <fstream>
#include <string_view>

/*
Loads hardware.json config file.
* Does NOT create config file if missing
*/
std::expected<nlohmann::json, Error> get_hardware_config() {
    std::ifstream f(CONFIG_PATH_HW);
    if (!f.is_open()) {
        return std::unexpected(Error(ErrorType::FileError,std::format("Could not open config file at {}",CONFIG_PATH_HW)));
    }

    nlohmann::json config = nlohmann::json::parse(f,nullptr,false);
    if (config.is_discarded()) {
        return std::unexpected(Error(ErrorType::ParseError,"Error parsing config file"));
    }
    f.close();
    return config;
}

/*
Load auto.json
*/
std::expected<nlohmann::json, Error> get_auto_config() {
    std::ifstream f(CONFIG_PATH_AUTO);
    if (!f.is_open()) {
        std::ofstream create(CONFIG_PATH_AUTO);
        if (!create.is_open()) {
            return std::unexpected(Error(FileError, std::format("Could not create auto.json at {}",CONFIG_PATH_AUTO)));
        }
        create.close();
    }

    nlohmann::json config = nlohmann::json::parse(f,nullptr,false);
    if (config.is_discarded()) {
        return std::unexpected(Error(ErrorType::ParseError,"Error parsing config file"));
    }

    f.open(CONFIG_PATH_AUTO);
    if (!f.is_open()) {
        std::unexpected(Error(ErrorType::FileError,std::format("Could not open config file at {}",CONFIG_PATH_AUTO)));
    }
    f.close();
    return config;
}


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