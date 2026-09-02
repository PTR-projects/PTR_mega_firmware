cmake_minimum_required(VERSION 3.16.0)
get_filename_component(EXTRA_COMPONENT_DIRS "components" ABSOLUTE)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ptr_mega)