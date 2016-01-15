option(ronove_build_tests "Build all of ronove's tests." OFF)

option(ronove_build_samples "Build ronove's ssamples programs." OFF)

option(ronove_link_pthread "Link ronove's library with pthread." ON)

cmake_minimum_required(VERSION 3.2)

project(ronove CXX C)

include_directories(include)

add_definitions(-Wall)
add_definitions(-Wextra)
add_definitions(-Werror)
add_definitions(--std=c++14)

file(GLOB ronove_sources
    "src/*.cpp"
)

add_library(${PROJECT_NAME} ${ronove_sources})

if (ronove_link_pthread)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()

if (ronove_build_samples)
    add_executable(timer samples/timer.cpp)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(timer ${PROJECT_NAME})
endif()
