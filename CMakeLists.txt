cmake_minimum_required(VERSION 3.10)
project(car_detection_app)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-video-1.0)

add_executable(car_detection
    src/
)

target_include_directories(car_detection PRIVATE
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(car_detection
    ${GST_LIBRARIES}
)