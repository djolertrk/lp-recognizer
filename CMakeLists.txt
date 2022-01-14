cmake_minimum_required(VERSION 3.10)

project(lp-recognizer)

add_executable(lp-recognizer driver.cpp)

target_link_libraries(lp-recognizer
    openalpr)
