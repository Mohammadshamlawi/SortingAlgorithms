cmake_minimum_required(VERSION 3.26)
project(SortingAlgorithms C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
include_directories(C:/Program\ Files/MySQL/MySQL\ Server\ 8.0/include)
link_directories(C:/Program\ Files/MySQL/MySQL\ Server\ 8.0/bin)

add_executable(SortingAlgorithms main.c)

target_link_libraries(SortingAlgorithms C:/Program\ Files/MySQL/MySQL\ Server\ 8.0/lib/libmysql.lib)
