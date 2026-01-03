cmake_minimum_required(VERSION 3.10)

project(Headphones)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)
find_package(Curses REQUIRED)

include_directories(${CURSES_INCLUDE_DIR})

add_executable(listen
  main.cpp
  eth_lookup.cpp
  display.cpp
)

target_link_libraries(listen
    PRIVATE
        fmt::fmt
        ${CURSES_LIBRARIES}
)

target_include_directories(listen
    PRIVATE
        ${CURSES_INCLUDE_DIRS}
)
