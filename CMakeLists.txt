cmake_minimum_required(VERSION 3.25)
project(smart_ptrs)

set(CMAKE_CXX_STANDARD 23)

add_executable(smart_ptrs main.cpp sw_fwd.h weak.h intrusive.h)
