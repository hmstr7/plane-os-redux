#pragma once

#include <string_view>
#include <expected>

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
Error object that consists of an ErrorType enum and the location (string literal) where the error has occured.
Warning: expensive! Not to use in performance-critical code.
*/
struct Error {
    ErrorType errtype;
    std::string_view where;
    Error(ErrorType errtype, std::string_view where): errtype(errtype), where(where) {};
};

typedef std::expected<void, Error> void_expensive;
typedef std::expected<void, ErrorType> void_cheap;