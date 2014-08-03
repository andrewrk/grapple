cmake_minimum_required(VERSION 2.8)
project(grapple)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)

find_package(SFML 2.1 COMPONENTS graphics window system)
if(SFML_FOUND)
  set(STATUS_SFML "OK")
else(SFML_FOUND)
  set(STATUS_SFML "not found")
endif(SFML_FOUND)

find_package(Box2D)
if(BOX2D_FOUND)
  set(STATUS_BOX2D "OK")
else(BOX2D_FOUND)
  set(STATUS_BOX2D "not found")
endif(BOX2D_FOUND)

add_executable(grapple ${SOURCES} ${HEADERS})
set_target_properties(grapple PROPERTIES
  COMPILE_FLAGS "-std=c++11 -pedantic -Werror -Wall -g")
target_link_libraries(grapple ${SFML_LIBRARIES} ${BOX2D_LIBRARY})
include_directories(${SFML_INCLUDE_DIR} ${BOX2D_INCLUDE_DIR})

message(
"System Dependencies\n"
"-------------------\n"
"* SFML                         : ${STATUS_SFML}\n"
"* Box2D                        : ${STATUS_BOX2D}\n"
)
