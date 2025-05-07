cmake_minimum_required(VERSION 3.28)
project(ZSO_PR1 C)

set(CMAKE_C_STANDARD 11)

add_executable(ZSO_PR1 main.c
        project_zso.c
)

target_link_libraries(ZSO_PR1 pthread)
