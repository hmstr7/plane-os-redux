#pragma once

#include <map>
#include <string_view>
#include <expected>
#include <string>
#include "spdlog/fmt/fmt.h"

/*
Error code enum.
Can be used as cheap error object
*/
enum ErrorType {
    DefaultError, // Unknown Error
    FileError,  // File not found / other
    ParseError, // Error during parsing (ex. json::parse()) 
    KeyError, // Missing key
    JsonTypeError, // JSON object is not of desired type
    TypeError, // Generic error concerning types
    InitError, // Generic error concerning initialization
    OutOfRangeError // self explanatory
};

/*
Error names
*/
inline const std::map<ErrorType, std::string> ErrorTypeToStr = {
    {DefaultError, "Unknown Error"},
    {FileError,"File Error"},
    {ParseError,"Parse Error"}, 
    {KeyError,"Key Not Found"},
    {JsonTypeError,"JSON object type mismatch"},
    {TypeError,"Type Error"},
    {InitError, "Init Error"},
    {OutOfRangeError, "Out Of Range Error"}
};

/*
Error object that consists of an ErrorType enum and the location (string literal) where the error has occured.
Warning: expensive! Not to use in performance-critical code.
*/
struct Error {
    ErrorType errtype;
    std::string where;
    Error(ErrorType errtype, std::string_view where): errtype(errtype), where(where) {};
    inline std::string to_string() const {
        return ErrorTypeToStr.at(errtype) + " @ " + where;
    }
};



template <>
struct fmt::formatter<Error> : fmt::formatter<std::string_view> {
    auto format(const Error& e, format_context& ctx) const {
        return fmt::formatter<std::string_view>::format(
            e.to_string(), ctx);
    }
};

typedef std::expected<void, Error> void_expensive;
typedef std::expected<void, ErrorType> void_cheap;