project(doorbell)

cmake_minimum_required(VERSION 3.0)

find_library(HACKRF_LIBRARY hackrf REQUIRED)

add_executable(hackrf_doorbell
    main.c
)

target_link_libraries(hackrf_doorbell
    ${HACKRF_LIBRARY}
)
