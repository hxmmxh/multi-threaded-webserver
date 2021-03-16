cmake_minimum_required (VERSION 3.10)

aux_source_directory(. WebServer_srcs)
add_library(WebServer STATIC ${WebServer_srcs})

aux_source_directory(Test Tests)

foreach(testfile ${Tests})
    get_filename_component(testname ${testfile} NAME_WE)
    add_executable(w${testname} ${testfile})
    target_link_libraries(w${testname} WebServer Reactor Sockets Timer Log Time Thread)
 endforeach(testfile)