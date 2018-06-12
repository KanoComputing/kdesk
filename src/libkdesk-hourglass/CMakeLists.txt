cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(kdesk-hourglass VERSION 0.1 LANGUAGES CXX)


add_library(kdesk-hourglass SHARED
	src/kdesk-hourglass.cpp
)

find_package(X11 COMPONENTS X11 Xft Xss REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(sn REQUIRED libstartup-notification-1.0)

target_include_directories(kdesk-hourglass
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include/kdesk>
	${X11_INCLUDE_DIR}
	${sn_INCLUDE_DIRS}
)

target_link_libraries(kdesk-hourglass
	PUBLIC ${sn_LIBRARIES} ${X11_LIBRARIES} kdesk
)

add_executable(kdesk-hourglass-app
	src/kdesk-hourglass-app.cpp
)

target_link_libraries(kdesk-hourglass-app
	PUBLIC kdesk-hourglass
)
