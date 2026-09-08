# Standalone on purpose: it links the ALREADY BUILT import library rather than
# the ta-lib target, so it proves what the shipped DLL exports and not what the
# build tree could resolve internally.
cmake_minimum_required(VERSION 3.15)
project(dll_probe C)

if(NOT TA_INCLUDE_DIR OR NOT TA_IMPORT_LIB)
	message(FATAL_ERROR "Set -DTA_INCLUDE_DIR=<repo>/include and -DTA_IMPORT_LIB=<path to ta-lib.lib>")
endif()

add_executable(dll_probe dll_probe.c)
target_include_directories(dll_probe PRIVATE "${TA_INCLUDE_DIR}")
target_link_libraries(dll_probe PRIVATE "${TA_IMPORT_LIB}")
