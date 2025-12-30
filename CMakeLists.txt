cmake_minimum_required(VERSION 3.10)

project(Headphones)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)

add_executable(listen
  main.cpp
  eth_lookup.cpp
)
target_link_libraries(listen PRIVATE fmt::fmt)
