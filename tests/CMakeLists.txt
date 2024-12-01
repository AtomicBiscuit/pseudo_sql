enable_testing()

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

file(GLOB test_source "cases/*.cpp")

configure_file(fixtures/fix1.bin ./../fixtures/fix1.bin COPYONLY)

add_executable(Test test_all.cpp ${test_source})

target_link_libraries(Test PRIVATE gtest_main Database)

target_include_directories(Test PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/include/command"
        "${PROJECT_SOURCE_DIR}/include/data"
)