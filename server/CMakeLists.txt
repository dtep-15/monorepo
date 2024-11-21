cmake_minimum_required(VERSION 3.16.0)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

get_filename_component(PARENT_DIR ../ ABSOLUTE)
add_subdirectory(${PARENT_DIR}/frontend ${CMAKE_BINARY_DIR}/frontend-build)

set(esp_target_name ${CMAKE_PROJECT_NAME}.elf)
if (TARGET ${esp_target_name})
	target_link_libraries(${esp_target_name} PRIVATE frontend)
endif()

get_filename_component(configName "${CMAKE_BINARY_DIR}" NAME)
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/.pio/libdeps/${configName}/esp_littlefs")

project(CurtainServer)
