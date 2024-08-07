cmake_minimum_required (VERSION 3.5)
project (comet C)

set(CMAKE_C_STANDARD 99)

add_subdirectory (third-party/httpc)

execute_process(
    COMMAND git submodule update --init --recursive --depth 1
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

file (GLOB COMET_SOURCES "src/*.c")

add_library (${PROJECT_NAME} ${COMET_SOURCES})
set (COMET_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories (${PROJECT_NAME} PUBLIC ${COMET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} httpc)

option (COMET_FORCE_BUILD_WINDOWS "Force build for Windows" OFF)

if (WIN32 OR COMET_FORCE_BUILD_WINDOWS)
    target_link_libraries (${PROJECT_NAME} ws2_32)
endif ()

option (COMET_BUILD_EXAMPLES "Build examples" ON)

if (COMET_BUILD_EXAMPLES)
    file (GLOB EXAMPLES "examples/*.c")
    foreach (EXAMPLE ${EXAMPLES})
        get_filename_component (EXAMPLE_NAME ${EXAMPLE} NAME_WE)
        add_executable (${EXAMPLE_NAME} ${EXAMPLE})
        target_link_libraries (${EXAMPLE_NAME} ${PROJECT_NAME})

        if (WIN32 OR COMET_FORCE_BUILD_WINDOWS)
            target_link_libraries (${PROJECT_NAME} ws2_32)
        endif ()

    endforeach ()
endif ()
