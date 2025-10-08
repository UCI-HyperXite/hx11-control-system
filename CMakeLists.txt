cmake_minimum_required(VERSION 3.10)
project(HX11_Control_System C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Debug sanitizers
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

add_executable(hx11 src/main.cpp)

enable_testing()
add_test(NAME SampleTest COMMAND hx11)
