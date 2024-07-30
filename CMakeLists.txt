cmake_minimum_required(VERSION 3.22)

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

if(DEFINED DEPENDENCIES_DIR)
	include(${DEPENDENCIES_DIR}/modorganizer_super/cmake_common/mo2.cmake)
else()
	include(${CMAKE_CURRENT_LIST_DIR}/cmake_common/mo2.cmake)
endif()

project(preview_nif)
set(project_type plugin)
set(enable_warnings OFF)

FetchContent_Declare(
	gli
	GIT_REPOSITORY https://github.com/g-truc/gli.git
	GIT_TAG master
)
set(GLI_TEST_ENABLE OFF CACHE BOOL "Build gli unit tests")

FetchContent_Declare(
	nifly
	GIT_REPOSITORY https://github.com/ousnius/nifly.git
	GIT_TAG main
)

FetchContent_MakeAvailable(gli nifly)

add_subdirectory(src)
target_link_libraries(preview_nif PRIVATE nifly gli)
