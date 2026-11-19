cmake_minimum_required(VERSION 3.16)
project(simulador_riscv_miguel C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(riscv-miguel src/main.c src/memory.c)
target_include_directories(riscv-miguel PRIVATE include)
