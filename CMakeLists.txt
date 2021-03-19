cmake_minimum_required (VERSION 3.10)
project(HxmWebServer)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
link_directories(${PROJECT_BINARY_DIR}/lib)

# file(GLOB_RECURSE Headers ${PROJECT_SOURCE_DIR} *.h)
# include_directories(${Headers})

include_directories(${PROJECT_SOURCE_DIR}/Time ${PROJECT_SOURCE_DIR}/Thread ${PROJECT_SOURCE_DIR}/Timer ${PROJECT_SOURCE_DIR}/Reactor  
                    ${PROJECT_SOURCE_DIR}/Log ${PROJECT_SOURCE_DIR}/WebServer ${PROJECT_SOURCE_DIR}/Sockets ${PROJECT_SOURCE_DIR}/http)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# set(CMAKE_C_FLAGS "-g -Wall -pthread")
set(CMAKE_CXX_FLAGS "-g -Wall -pthread")

add_subdirectory(Time)
add_subdirectory(Thread)
add_subdirectory(Log)
add_subdirectory(Sockets)
add_subdirectory(Timer)
add_subdirectory(Reactor)
add_subdirectory(WebServer)
add_subdirectory(Example)
add_subdirectory(chat)
# add_subdirectory(http)

