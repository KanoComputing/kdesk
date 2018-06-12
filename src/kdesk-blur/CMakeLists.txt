cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(kdesk-blur VERSION 0.1 LANGUAGES CXX)

add_executable(kdesk-blur
	src/kdesk-blur.cpp
)

if (CMAKE_VERSION VERSION_LESS "3.8")
	set(CMAKE_CXX_STANDARD 11)
else()
	target_compile_features(kdesk-app PRIVATE cxx_std_11)
endif()

target_compile_options(kdesk-blur PRIVATE -Wall)

find_package(PkgConfig REQUIRED)
pkg_check_modules(imlib2 REQUIRED imlib2)
find_package(Threads REQUIRED)
find_package(X11 COMPONENTS X11 Xft Xss REQUIRED)

include_directories(${X11_INCLUDE_DIR})

target_include_directories(kdesk-blur
	PRIVATE include
)

target_link_libraries(kdesk-blur
	PUBLIC Imlib2 Threads::Threads ${X11_LIBRARIES} kdesk
)
