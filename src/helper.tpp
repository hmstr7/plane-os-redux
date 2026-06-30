#pragma once
#include <expected>
#include "errors.hpp"
#include "../extern/json/single_include/nlohmann/json.hpp"

template<typename T>
std::expected<T, Error> get_from_json(
    const nlohmann::json& json, 
    std::initializer_list<std::string_view> path, 
    std::string_view caller
) { 
    const nlohmann::json* current_chunk = &json;
    for (auto key : path) {
        auto it = current_chunk->find(key);
        if (it == current_chunk->end()) {
            return std::unexpected(Error(ErrorType::KeyError,caller));
        }
        current_chunk = &(*it);
    }
    if constexpr (std::is_same_v<T, int>) {
        if (!current_chunk->is_number_integer())
            return std::unexpected(Error(ErrorType::JsonTypeError, caller));
        return current_chunk->get<int>();
    }

    if constexpr (std::is_same_v<T, float>) {
        if (!current_chunk->is_number())
            return std::unexpected(Error(ErrorType::JsonTypeError, caller));
        return current_chunk->get<float>();
    }

    if constexpr (std::is_same_v<T, bool>) {
        if (!current_chunk->is_boolean())
            return std::unexpected(Error(ErrorType::JsonTypeError, caller));
        return current_chunk->get<bool>();
    }

    if constexpr (std::is_same_v<T, std::string>) {
        if (!current_chunk->is_string())
            return std::unexpected(Error(ErrorType::JsonTypeError, caller));
        return current_chunk->get<std::string>();
    }

    if (sizeof(T) == 0) {
        return std::unexpected(Error(ErrorType::TypeError,caller));
    }
}