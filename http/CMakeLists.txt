cmake_minimum_required (VERSION 3.10)

aux_source_directory(. Http_srcs)
add_library(Http STATIC ${Http_srcs})

aux_source_directory(Test Tests)

foreach(testfile ${Tests})
    get_filename_component(testname ${testfile} NAME_WE)
    add_executable(${testname} ${testfile})
    target_link_libraries(${testname} Http WebServer Reactor Sockets Timer Log Time Thread)
 endforeach(testfile)