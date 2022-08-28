cmake_minimum_required(VERSION 3.21)
project(lua_density C)

set(CMAKE_C_STANDARD 11)

include_directories(E:/Anaconda3/Library/include ./dep/src)
link_directories("E:/Anaconda3/Library/lib" E:/Anaconda3/Library/bin)

file(GLOB_RECURSE SOURCES  ./dep/src/*.c) #./dep/src/structure/*.c ./dep/src/buffers/*.c ./dep/src/algorithms/*.c)
message(${SOURCES})

add_library(lua_density SHARED src/ldensity.c ${SOURCES})
target_link_libraries(lua_density lua)

set_target_properties(lua_density PROPERTIES OUTPUT_NAME density)