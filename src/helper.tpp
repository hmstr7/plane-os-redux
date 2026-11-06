#pragma once
#include <expected>
#include <iterator>
#include <string>
#include "errors.hpp"
#include "../extern/json/single_include/nlohmann/json.hpp"
#include <fstream>
#include <string_view>
#include <type_traits>
#include <format>
#include <utility>
#include <initializer_list>

typedef nlohmann::json json_t;
using json = nlohmann::json; 

inline std::string init_list_to_string(std::initializer_list<std::string_view> list) {
    std::string list_str;

    for (const auto& key : list) {
        std::string escaped;

        for (char c : key) {
            if (c == '~') escaped += "~0";
            else if (c == '/') escaped += "~1";
            else escaped += c;
        }

        list_str += "/" + escaped;
    }

    return list_str;
}

/**
 * Get a value from a json file
 * 
 * @details Return a value from a json file at a specified path (nested objects). File must be readable and a valid json.
 * 
 * @tparam T must be either an int, unsigned int, float, std::string or a bool 
 * TODO: support more types
 * 
 * @param filepath path to the json file
 * @param keypath a path to the desired value made of nested keys
 * 
 * @return requested value or Error
 */
template<typename T>
std::expected<T, Error> get_from_json(
    std::string filepath,
    std::initializer_list<std::string_view> keypath
) { 
    if (keypath.size() == 0) {
        return std::unexpected(Error(KeyError, "invalid key"));
    }
    std::string keypath_str = init_list_to_string(keypath);
    std::ifstream filestream_read(filepath);

    if (!filestream_read.is_open() || !filestream_read.good()) {
        if (filestream_read.is_open() && !filestream_read.good()) {
            filestream_read.close();
            return std::unexpected(Error(FileError, std::format("error opening file \"{}\"",filepath)));
        }
        return std::unexpected(Error(FileError, std::format("file not found \"{}\"",filepath)));
    }

    json_t json_o = json::parse(filestream_read, nullptr, false);
    if (json_o.is_discarded()) {
        return std::unexpected(Error(ParseError, std::format("error parsing json file \"{}\"",filepath)));
    }

    filestream_read.close();

    // Find the key
    json_t* current_chunk = &json_o;
    json_t raw_out;

    
    for (auto key = keypath.begin(); key != keypath.end(); ++key) {
        auto it = current_chunk->find(*key);
        if (it == current_chunk->end()) {
            return std::unexpected(Error(KeyError, std::format("key {} not found in \"{}\"", *key, filepath)));
        }
        if (key == std::prev(keypath.end())) {
            raw_out = *it;
        } else {
            if (!it->is_object()) {
                return std::unexpected(Error(KeyError, std::format("key {} doesn\'t contain a nested object and is not final", *key)));
            }
            current_chunk = &(*it);
        }
    }
    
    if constexpr(std::is_same_v<T, int>) { // ? std::remove_cvref_t<T> maybe?
        // signed integer
        if (raw_out.is_number_integer()) { // ? should floats be alowed
            return raw_out.get<int>();
        } else {
            return std::unexpected(Error(TypeError, std::format("\"{}\" is not an integer",keypath_str)));
        }
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        // unsigned integer
        if (raw_out.is_number_unsigned()) { // ? should floats be alowed
            return raw_out.get<unsigned int>();
        } else {
            return std::unexpected(Error(TypeError, std::format("\"{}\" is not an unsigned integer",keypath_str)));
        }
    } else if constexpr (std::is_same_v<T,float>) {
        // float
        if (raw_out.is_number()) {
            return raw_out.get<float>();
        } else {
            return std::unexpected(Error(TypeError, std::format("\"{}\" is not a float",keypath_str)));
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
        // string
        if (raw_out.is_string()) {
            return raw_out.get<T>();
        } else {
            return std::unexpected(Error(TypeError, std::format("\"{}\" is not a string",keypath_str)));
        }
    } else if constexpr (std::is_same_v<T, bool>) {
        // unsigned integer
        if (raw_out.is_boolean()) {
            return raw_out.get<bool>();
        } else {
            return std::unexpected(Error(TypeError, std::format("\"{}\" is not a boolr",keypath_str)));
        }
    } else {
        return std::unexpected(Error(TypeError, "type not supported"));
    }
}

template <typename T>
concept JsonAssignable = requires(json_t& json_o, T& value) {
    json_o = std::forward<T>(value);
};

/**
 * Write a value to a json file
 * 
 * Create or update a value at a specified (nested) key in a json file. Creates nested json objects if necessary. File must be readable, writable and be a valid json
 * 
 * @tparam T must be a type compatible with json (convertible to nlohmann::json::basic_json)
 * 
 * @param filepath path to the json file
 * @param keypath a path to write the value at made of nested keys
 * @param value value to write of type convertible to basic_json 
 * 
 * @return nothing or Error
 * 
 * @code
 * // usage example
 * write_to_json<float>(CONFIG_PATH_AUTO, {"imu_gyro_bias_x"}, -1.5);
 * @endcode
 */
template <typename T>
requires JsonAssignable<T>
void_expensive write_to_json(
    std::string filepath,
    std::initializer_list<std::string_view> keypath,
    const T& value
) {
    if (keypath.size() == 0) {
        return std::unexpected(Error(KeyError, "invalid key"));
    }
    std::string keypath_str = init_list_to_string(keypath);

    std::ifstream filestream_read(filepath);

    json_t json_o;

    if (!filestream_read.is_open() || !filestream_read.good()) {
        if (filestream_read.is_open() && !filestream_read.good()) {
            filestream_read.close();
            return std::unexpected(Error(FileError, std::format("error opening file \"{}\"",filepath)));
        } else {
            filestream_read.open(filepath, std::ios::out | std::ios::trunc);
            if (!filestream_read.is_open() || !filestream_read.good()) {
                if (filestream_read.is_open()) filestream_read.close();
                return std::unexpected(Error(FileError, std::format("could not create file \"{}\"",filepath)));
            }
            filestream_read.close();
            filestream_read.open(filepath, std::ios::in | std::ios::out);
            if (!filestream_read.is_open() || !filestream_read.good()) {
                if (filestream_read.is_open()) filestream_read.close();
                return std::unexpected(Error(FileError, std::format("could not create file \"{}\"",filepath)));
            }
            json_o = {};
        }
    } else {
        json_o = json::parse(filestream_read, nullptr, false);
    }

    filestream_read.close();
    if (filestream_read.fail()) {
        return std::unexpected(Error(
            FileError,
            std::format("error closing file \"{}\"", filepath)
        ));
    }
    
    if (json_o.is_discarded()) {
        return std::unexpected(Error(ParseError, std::format("error parsing json file \"{}\"",filepath)));
    }
    
    // check for primitives in the middle of a keypath
    json_t* current_chunk = &json_o;
    for (auto key = keypath.begin(); key != keypath.end(); ++key) {
        auto it = current_chunk->find(*key);
        if (it == current_chunk->end()) {
            break;
        }
        if (!it->is_object() && !(key == std::prev(keypath.end()))) {
            return std::unexpected(Error(KeyError, std::format("existing key {} is not final in \"{}\" but is not an object", *key,keypath_str)));
        }
        
        current_chunk = &(*it);
        
    }

    json_o[json::json_pointer(keypath_str)] = value; // ! may throw exceptions or abort -> fix using json_noexceptions and define custom throw

    std::ofstream filestream_write(filepath, std::ios::trunc);
    if (!filestream_write.is_open() || !filestream_write.good()) {
        if (filestream_write.is_open()) filestream_write.close();
        return std::unexpected(Error(FileError, std::format("could not write to file \"{}\"",filepath)));
    }

    filestream_write << json_o.dump(4);

    if (!filestream_write.good()) {
        filestream_write.close();
        return std::unexpected(Error(
            FileError,
            std::format("error writing file \"{}\"", filepath)
        ));
    }

    filestream_write.close();

    if (filestream_write.fail()) {
        return std::unexpected(Error(
            FileError,
            std::format("error closing file \"{}\"", filepath)
        ));
    }
    return {};
}