cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(kdesk VERSION 0.1 LANGUAGES CXX)

add_executable(kdesk-app
	src/main.cpp
	src/background.cpp
	src/desktop.cpp
	src/grid.cpp
	src/icon.cpp
	src/sound.cpp
	src/ssaver.cpp
)
set_target_properties(kdesk-app PROPERTIES OUTPUT_NAME kdesk)

if (CMAKE_VERSION VERSION_LESS "3.8")
	set(CMAKE_CXX_STANDARD 11)
else()
	target_compile_features(kdesk-app PRIVATE cxx_std_11)
endif()

target_compile_options(kdesk-app PRIVATE -Wall)


target_include_directories(kdesk-app
	PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include/kdesk>
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(imlib2 REQUIRED imlib2)
find_package(X11 COMPONENTS X11 Xft Xss Xscreensaver REQUIRED)
find_package(Threads REQUIRED)
find_package(Freetype REQUIRED)

include_directories(${X11_INCLUDE_DIR})
include_directories(${FREETYPE_INCLUDE_DIRS})

target_link_libraries(kdesk-app
	PRIVATE Imlib2 Threads::Threads ${X11_LIBRARIES} ${X11_Xscreensaver_LIB} ${X11_Xft_LIB} ${FREETYPE_LIBRARIES} kdesk kdesk-hourglass
)
