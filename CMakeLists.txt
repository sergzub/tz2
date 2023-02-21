cmake_minimum_required(VERSION 3.16)

project(tz2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_GENERATOR_PLATFORM x64)

add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    trace.cpp
    pull.cpp
    alloc_impl.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Threads::Threads)
