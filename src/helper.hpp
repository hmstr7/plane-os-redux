#pragma once

#include <expected>
#include "../extern/json/single_include/nlohmann/json.hpp"
#include "errors.hpp"

#define CONFIG_PATH_HW "../config/hardware.json"
#define CONFIG_PATH_AUTO "../config/hardware.json"



std::expected<nlohmann::json, Error> get_hardware_config();
std::expected<nlohmann::json, Error> get_auto_config();

template<typename T>
std::expected<T, Error> get_from_json(
    const nlohmann::json& json, 
    std::initializer_list<std::string_view> path
);
#include "helper.tpp"

std::expected<uint8_t, ErrorType> to_uint8(int value);
std::expected<uint16_t, ErrorType> to_uint16(int value);
std::expected<uint32_t, ErrorType> to_uint32(int value);