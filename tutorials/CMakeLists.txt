cmake_minimum_required(VERSION 3.5)

project(tutorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compile mode to debug
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(ENABLE_GTK "Build tools" OFF)

if (ENABLE_GTK)

    # Use the package PkgConfig to detect GTK+ headers/library files
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    # Setup CMake to use GTK+, tell the compiler where to look for headers
    # and to the linker where to look for libraries
    INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
    LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})
    # Add other flags to the compiler
    ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})
    message(STATUS "GTK3_INCLUDE_DIRS: ${GTK3_INCLUDE_DIRS}")
    message(STATUS "GTK3_LIBRARY_DIRS: ${GTK3_LIBRARY_DIRS}")
    message(STATUS "GTK3_CFLAGS_OTHER: ${GTK3_CFLAGS_OTHER}")


    # Link the target to the GTK+ libraries
    add_executable(ch5_gui_toolkit ch5_gui_toolkit.cpp)
    target_link_libraries(ch5_gui_toolkit ${GST_LIBRARIES} ${GTK3_LIBRARIES})
endif()


add_executable(ch2_concepts ch2_concepts.cpp)
target_link_libraries(ch2_concepts ${GST_LIBRARIES})


add_executable(ch3_dynamic_pipelines_pad ch3_dynamic_pipelines_pad.cpp)
target_link_libraries(ch3_dynamic_pipelines_pad ${GST_LIBRARIES})

add_executable(ch4_time_manage_seeking ch4_time_manage_seeking.cpp)
target_link_libraries(ch4_time_manage_seeking ${GST_LIBRARIES})


add_executable(ch6_media_format_and_pad_capabilities ch6_media_format_and_pad_capabilities.cpp)
target_link_libraries(ch6_media_format_and_pad_capabilities ${GST_LIBRARIES})

add_executable(ch7_multithread_pad_availability ch7_multithread_pad_availability.cpp)
target_link_libraries(ch7_multithread_pad_availability ${GST_LIBRARIES})

add_executable(ch8_short_cutting_the_pipeline ch8_short_cutting_the_pipeline.cpp)
target_link_libraries(ch8_short_cutting_the_pipeline ${GST_LIBRARIES})

add_executable(ch9_media_info_gathering ch9_media_info_gathering.cpp)
target_link_libraries(ch9_media_info_gathering ${GST_LIBRARIES})

add_executable(ch12_streaming ch12_streaming.cpp)
target_link_libraries(ch12_streaming ${GST_LIBRARIES})

add_executable(rtspsrc_example rtspsrc_example.cpp)
target_link_libraries(rtspsrc_example ${GST_LIBRARIES})