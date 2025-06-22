cmake_minimum_required(VERSION 3.10)
project(goodix_driver C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

file(GLOB SRC_FILES src/*.c)

add_executable(goodix_driver ${SRC_FILES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
target_link_libraries(goodix_driver ${LIBUSB_LIBRARIES})

