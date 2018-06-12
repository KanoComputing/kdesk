cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(kdesk-eglsaver VERSION 0.1 LANGUAGES CXX)


# TODO: Convert target to properly use CMake
add_custom_target(
    kdesk-eglsaver ALL
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}
)
