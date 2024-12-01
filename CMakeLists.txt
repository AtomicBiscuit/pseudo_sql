cmake_minimum_required(VERSION 3.22)
project(pseudo_sql)

set(CMAKE_CXX_STANDARD 23)
# set(COMPILE_FLAGS "--coverage")
# set(CMAKE_EXE_LINKER_FLAGS "--coverage")

add_executable(pseudo_sql main.cpp)


add_subdirectory(src)
add_subdirectory(tests)

target_link_libraries(pseudo_sql PUBLIC Database)