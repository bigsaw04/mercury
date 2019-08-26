cmake_minimum_required(VERSION 2.8)
project(mercury)
set(CMAKE_CXX_STANDARD 17)
include_directories(/home/chris/oss-include)

add_executable(coinbase-robot "coinbase.cpp")
find_package(Boost 1.67 COMPONENTS thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

target_link_libraries(coinbase-robot ${Boost_LIBRARIES} ssl crypto pthread cpprest stdc++fs asound sndfile magic)





