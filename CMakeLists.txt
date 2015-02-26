cmake_minimum_required (VERSION 2.8)
project (robe)

FIND_PACKAGE (Threads) 

message (INFO " found libmraa version: ${MRAA_LIBRARIES}")

# Appends the cmake/modules path to MAKE_MODULE_PATH variable.
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include_directories (
  ${PROJECT_SOURCE_DIR}/include
)

link_directories (
  ${PROJECT_SOURCE_DIR}/libs
)

add_subdirectory (src) 