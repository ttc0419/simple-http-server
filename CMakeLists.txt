cmake_minimum_required(VERSION 3.20)
project(
    simple-http-server
    VERSION 1.0.0
    DESCRIPTION "A simple Linux HTTP server implemented using epoll and io_uring for benchmark"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -fsanitize=address")

add_executable(simple-http-server-epoll src/epoll.c src/queue.h src/queue.c src/common.h)
target_link_libraries(simple-http-server-epoll pthread)

add_executable(simple-http-server-uring src/uring.c src/common.h)
target_link_libraries(simple-http-server-uring uring)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)

    if(supported)
        set_target_properties(simple-http-server-epoll simple-http-server-uring PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO is not supported: ${error}")
    endif()
endif()
