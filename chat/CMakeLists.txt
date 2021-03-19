cmake_minimum_required (VERSION 3.10)

aux_source_directory(. Tests)

foreach(testfile ${Tests})
    get_filename_component(testname ${testfile} NAME_WE)
    add_executable(${testname} ${testfile} codec.h)
    target_link_libraries(${testname} WebServer Reactor Sockets Timer Log Time Thread)
 endforeach(testfile)