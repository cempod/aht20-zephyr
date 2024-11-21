cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aht20-zephyr)

set(SCRS
    ${CMAKE_CURRENT_LIST_DIR}/src/monocraft.c
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_sources(app PRIVATE ${SCRS})
