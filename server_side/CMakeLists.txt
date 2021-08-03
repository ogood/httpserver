ADD_SUBDIRECTORY(tcp_service)
ADD_SUBDIRECTORY(epoll_manager)

aux_source_directory(. server_side_SRCS)

add_library(server_side ${server_side_SRCS} )




TARGET_LINK_LIBRARIES(server_side
epoll_manager
tcp_service
 )





