cmake_minimum_required (VERSION 3.10)

aux_source_directory(. Http_srcs)
add_library(Http STATIC ${Time_srcs})

add_subdirectory(Test)