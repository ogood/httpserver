cmake_minimum_required(VERSION 3.16)
project(server C CXX)
set(PROJECT_NAME server )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(project_root ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
${project_root}
)


set(CMAKE_BUILD_TYPE "Debug")

ADD_SUBDIRECTORY(server_side )
ADD_SUBDIRECTORY(common )

add_executable(server "test.cpp" )
TARGET_LINK_LIBRARIES(${PROJECT_NAME}

server_side
)



