cmake_minimum_required(VERSION 3.10)
project(testRedisProtocol)

set(CMAKE_CXX_STANDARD 11)

add_executable(testRedisProtocol main.cpp define.h server.c server.h client.c client.h util.c sds.c)
target_link_libraries(testRedisProtocol pthread)