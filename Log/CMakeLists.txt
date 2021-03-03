cmake_minimum_required (VERSION 3.10)

aux_source_directory(. Log_srcs)
add_library(Log STATIC ${Log_srcs})

add_subdirectory(Test)