cmake_minimum_required(VERSION 3.5)

project(test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compile mode to debug
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(test_rtspsrc test_rtspsrc.cpp)
target_link_libraries(test_rtspsrc ${GST_LIBRARIES})

add_executable(test_rtspsrc_cmd test_rtspsrc_cmd.cpp)
target_link_libraries(test_rtspsrc_cmd ${GST_LIBRARIES})

add_executable(test_my_gst_camera test_my_gst_camera.cpp)
target_link_libraries(test_my_gst_camera gstcamera)
