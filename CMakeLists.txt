# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# Register all custom components
# These components follow ESP-IDF component structure and conventions
set(EXTRA_COMPONENT_DIRS

    ${CMAKE_CURRENT_LIST_DIR}/components/driver
    ${CMAKE_CURRENT_LIST_DIR}/components/system
    ${CMAKE_CURRENT_LIST_DIR}/components/apps
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Trim the build - include only minimal set of components and their dependencies
idf_build_set_property(MINIMAL_BUILD ON)
project(ate_console_app)




