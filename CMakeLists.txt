cmake_minimum_required(VERSION 3.13)
project(VsomeipSample)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vsomeip3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${vsomeip3_INCLUDE_DIRS}
)

add_executable(server src/server.cpp)
target_link_libraries(server vsomeip3 ${Boost_LIBRARIES})

add_executable(client src/client.cpp)
target_link_libraries(client vsomeip3 ${Boost_LIBRARIES})
