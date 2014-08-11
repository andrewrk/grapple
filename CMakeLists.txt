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

find_package(tmxparser)
if(TMXPARSER_FOUND)
  set(STATUS_TMXPARSER "OK")
else(TMXPARSER_FOUND)
  set(STATUS_TMXPARSER "not found")
endif(TMXPARSER_FOUND)

find_package(RuckSack)
if(RUCKSACK_FOUND)
  set(STATUS_RUCKSACK "OK")
else(RUCKSACK_FOUND)
  set(STATUS_RUCKSACK "not found")
endif(RUCKSACK_FOUND)

add_executable(grapple ${SOURCES} ${HEADERS})
set_target_properties(grapple PROPERTIES
  COMPILE_FLAGS "-std=c++11 -pedantic -Werror -Wall -g")
target_link_libraries(grapple
  ${SFML_LIBRARIES}
  ${BOX2D_LIBRARY}
  ${TMXPARSER_LIBRARY}
  ${RUCKSACK_LIBRARY}
  )
include_directories(
  ${SFML_INCLUDE_DIR}
  ${BOX2D_INCLUDE_DIR}
  ${TMXPARSER_INCLUDE_DIR}
  ${RUCKSACK_INCLUDE_DIR}
  )
add_dependencies(grapple assets)

# always run rucksack; it has its own mtime checking.
set(ASSETS_BUNDLE "${CMAKE_BINARY_DIR}/assets.bundle")
set(SYMBOLIC_ASSETS_BUNDLE "${ASSETS_BUNDLE}.")
set_source_files_properties(${SYMBOLIC_ASSETS_BUNDLE} PROPERTIES SYMBOLIC true)
add_custom_command(OUTPUT ${SYMBOLIC_ASSETS_BUNDLE}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMAND ${RUCKSACK_EXECUTABLE}
  ARGS "bundle" "${CMAKE_SOURCE_DIR}/assets.json" ${ASSETS_BUNDLE})
add_custom_target(assets DEPENDS ${SYMBOLIC_ASSETS_BUNDLE})

message(
"System Dependencies\n"
"-------------------\n"
"* SFML                         : ${STATUS_SFML}\n"
"* Box2D                        : ${STATUS_BOX2D}\n"
"* tmxparser                    : ${STATUS_TMXPARSER}\n"
"* rucksack                     : ${STATUS_RUCKSACK}\n"
)
