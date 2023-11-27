cmake_minimum_required(VERSION 3.22)

if(DEFINED DEPENDENCIES_DIR)
	include(${DEPENDENCIES_DIR}/modorganizer_super/cmake_common/mo2.cmake)
else()
	include(${CMAKE_CURRENT_LIST_DIR}/cmake_common/mo2.cmake)
endif()

project(preview_nif)
set(project_type plugin)
set(enable_warnings OFF)

add_subdirectory(src)
set(GLI_TEST_ENABLE OFF CACHE BOOL "Build gli unit tests")
add_subdirectory(external/gli)
add_subdirectory(external/nifly)
target_link_libraries(preview_nif PRIVATE nifly gli)
