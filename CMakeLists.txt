cmake_minimum_required(VERSION 4.1)
project(do C)

set(CMAKE_C_STANDARD 99)
set(ASAN "Enable address analyzer" OFF)

if (ASAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address \
        -fsanitize-recover=address \
        -fsanitize-address-use-after-scope \
        -fno-omit-frame-pointer \
        -fno-inline \
        -fsanitize=undefined \
        -g -O0"
    )

    add_link_options(-fsanitize=address)
    add_link_options(-lubsan)
endif()

add_executable(do main.c
        str.c
        str.h
        targetlist.c
        targetlist.h
        common.h
        target.c
        target.h)
