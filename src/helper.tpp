#pragma once
#include <expected>
#include <iterator>
#include <string>
#include "errors.hpp"
#include "../extern/json/single_include/nlohmann/json.hpp"
#include <fstream>
using json = nlohmann::json; 

template<typename T>
std::expected<T, Error> get_from_json(
    const json& json, 
    std::initializer_list<std::string_view> path
) { 
    const ::json* current_chunk = &json;
    std::string_view k = *std::prev(path.end());

    for (auto key : path) {
        auto it = current_chunk->find(key);
        if (it == current_chunk->end()) {
            return std::unexpected(Error(ErrorType::KeyError, std::format("Key {} not found",k)));
        }
        current_chunk = &(*it);
    }
    if constexpr (std::is_same_v<T, int>) {
        if (!current_chunk->is_number_integer())
            return std::unexpected(Error(ErrorType::JsonTypeError, std::format("Field {} must be an integer",k)));
        return current_chunk->get<int>();
    }

    if constexpr (std::is_same_v<T, float>) {
        if (!current_chunk->is_number())
            return std::unexpected(Error(ErrorType::JsonTypeError, std::format("Field {} must be a float",k)));
        return current_chunk->get<float>();
    }

    if constexpr (std::is_same_v<T, bool>) {
        if (!current_chunk->is_boolean())
            return std::unexpected(Error(ErrorType::JsonTypeError, std::format("Field {} must be a bool",k)));
        return current_chunk->get<bool>();
    }

    if constexpr (std::is_same_v<T, std::string>) {
        if (!current_chunk->is_string())
            return std::unexpected(Error(ErrorType::JsonTypeError, std::format("Field {} must be a string",k)));
        return current_chunk->get<std::string>();
    }

    if (sizeof(T) == 0) {
        return std::unexpected(Error(ErrorType::TypeError,"Invalid type"));
    }
}

template <typename T>
void_expensive write_to_json(
    json& json,
    const std::filesystem::path& file,
    std::initializer_list<std::string_view> path,
    const T& value
) {
    ::json* current = &json;

    for (auto key : path)
    {
        if (!current->is_object())
            return std::unexpected(Error(FileError,"Invalid config"));

        auto it = current->find(key);

        if (it == current->end())
            it = current->emplace(std::string(key), json::object()).first;

        current = &it.value();
    }

    *current = value;

    std::ofstream out(file);

    if (!out.is_open()) {
        return std::unexpected(Error(FileError, std::format("Failed to open file {}", file.string())));
    }

    out << json.dump(4) << '\n';
    if (!out) {
        return std::unexpected(Error(FileError,std::format("Could not write to file {}",file.string())));
    }
    
    return {};
}