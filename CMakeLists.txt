cmake_minimum_required(VERSION 3.5)

project(hello LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compile mode to debug
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4
                               gstreamer-sdp-1.0>=1.4
                               gstreamer-video-1.0>=1.4
                               gstreamer-app-1.0>=1.4
                               gstreamer-audio-1.0>=1.4
                               gstreamer-pbutils-1.0>=1.4)
# - target_include_directories with ${GST_INCLUDE_DIRS}
# - target_compile_options with ${GST_CFLAGS}
# - target_link_libraries with ${GST_LIBRARIES}
message(STATUS "GST_INCLUDE_DIRS: ${GST_INCLUDE_DIRS}")
message(STATUS "GST_CFLAGS: ${GST_CFLAGS}")
message(STATUS "GST_LIBRARIES: ${GST_LIBRARIES}")

include_directories( 
    ${GST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/utils
    ${CMAKE_SOURCE_DIR}/include/cuda
    )


set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# list(APPEND SRC_FILES ${SRC_DIR}/gst_camera.cpp ${SRC_DIR}/utils/mt_utils.cpp)
file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB CU ${CMAKE_SOURCE_DIR}/cu/*.cu)

find_package(CUDA)
if(CUDA_FOUND)
    message("-- CUDA version: ${CUDA_VERSION}")
    message("-- CUDA Include: ${CUDA_INCLUDE_DIRS}")
    include_directories(${CUDA_INCLUDE_DIRS})
    set(
        CUDA_NVCC_FLAGS
        ${CUDA_NVCC_FLAGS}; 
        -O3 
        -gencode arch=compute_37,code=sm_37
        -gencode arch=compute_53,code=sm_53
        -gencode arch=compute_60,code=sm_60
        -gencode arch=compute_61,code=sm_61
        -gencode arch=compute_62,code=sm_62
    )

    if(CUDA_VERSION_MAJOR GREATER 9)
        message("-- CUDA ${CUDA_VERSION_MAJOR} detected, enabling SM_72")

        set(
            CUDA_NVCC_FLAGS
            ${CUDA_NVCC_FLAGS}; 
            -gencode arch=compute_72,code=sm_72
        )
    endif()

    cuda_add_library(gstcamera SHARED ${SRC_FILES} ${CU})
    # target_link_libraries(gstcamera pthread GL GLEW gstreamer-1.0 gstapp-1.0 gobject-2.0 glib-2.0 ${CUDA_LIBRARIES})
    target_link_libraries(gstcamera pthread GL GLEW ${GST_LIBRARIES} ${CUDA_LIBRARIES})

    # cuda_add_executable(test_camera test_camera.cpp ${SRC} ${CU})
    # target_link_libraries(test_camera GL GLEW gstreamer-1.0 gstapp-1.0 gobject-2.0 glib-2.0 ${CUDA_LIBRARIES})

    add_subdirectory(test)
else()
    message("-- Can not find CUDA")
endif()


add_subdirectory(tutorials)
