



file(GLOB httpserver_SRC
     "*.cpp"
)
find_package(Threads)
#add_compile_definitions(GLOG_NO_ABBREVIATED_SEVERITIES)
add_library(tcp_service STATIC ${httpserver_SRC} )

#add_executable(history_service STATIC ${httpserver_SRC} )
target_link_libraries(tcp_service

epoll_manager
common
${CMAKE_THREAD_LIBS_INIT}
)
