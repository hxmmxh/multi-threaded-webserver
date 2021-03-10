cmake_minimum_required (VERSION 3.10)

aux_source_directory(. WebServer_srcs)
add_library(WebServer STATIC ${Thread_srcs})

add_subdirectory(Test)