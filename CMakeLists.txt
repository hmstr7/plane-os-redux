cmake_minimum_required(VERSION 3.4...3.18)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

project(plane-os-redux LANGUAGES C CXX)

# Global packages
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(Boost REQUIRED) # Global because too general and too big

file(GLOB SENSORIMU_SOURCES "extern/sensor-imu/*.c")
add_library(sensor-imu STATIC
    ${SENSORIMU_SOURCES}
)

# Git submodules
#add_subdirectory(extern/pybind11)
add_subdirectory(extern/ibus)
add_subdirectory(extern/PiPCA9685)
add_subdirectory(extern/json)
add_subdirectory(extern/spdlog)

add_executable(main
    # Main program
    src/main.cpp
    src/core.cpp

    # Headers
    src/core.hpp
    src/common.hpp
    src/errors.hpp
    src/helper.hpp
    src/helper.tpp
    src/io.hpp
    src/process.hpp

    # Modules
    src/helper.cpp
    src/io.cpp
    src/process.cpp
)

target_link_libraries(main PRIVATE ibus)
target_link_libraries(main PRIVATE PiPCA9685::PiPCA9685)
target_link_libraries(main PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(main PRIVATE spdlog::spdlog)
target_link_libraries(main PRIVATE Boost::headers)
target_link_libraries(main PRIVATE sensor-imu)