cmake_minimum_required (VERSION 3.10)
project(HxmWebServer)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
link_directories(${PROJECT_BINARY_DIR}/lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_C_FLAGS "-g -Wall")
# set(CMAKE_CXX_FLAGS "-g -Wall")

add_subdirectory(Time)
add_subdirectory(WebServer)
add_subdirectory(Thread)
add_subdirectory(Log)
add_subdirectory(http)

