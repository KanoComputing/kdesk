cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(libkdesk VERSION 0.1 LANGUAGES CXX)

# Initially just generate a static library for ease of integration
add_library(kdesk STATIC
	src/configuration.cpp
)

target_include_directories(kdesk
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include/kdesk>
)
