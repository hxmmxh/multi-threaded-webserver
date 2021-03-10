cmake_minimum_required (VERSION 3.10)

aux_source_directory(. Timer_srcs)
add_library(Timer STATIC ${Timer_srcs})

aux_source_directory(Test Tests)

foreach(testfile ${Tests})
    get_filename_component(testname ${testfile} NAME_WE)
    add_executable(${testname} ${testfile})
    target_link_libraries(${testname} Reactor Sockets Timer Log Time Thread)
 endforeach(testfile)