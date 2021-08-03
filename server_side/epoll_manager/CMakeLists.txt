#file(GLOB epollmnger_SRC     "*.cpp")
aux_source_directory(. epollmnger_SRC)
add_library(epoll_manager STATIC ${epollmnger_SRC} )
