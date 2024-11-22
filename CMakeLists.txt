cmake_minimum_required(VERSION 3.22)
project(pseudo_sql)

set(CMAKE_CXX_STANDARD 23)

add_executable(pseudo_sql main.cpp)

add_subdirectory(src)

target_link_libraries(pseudo_sql PUBLIC Database)
