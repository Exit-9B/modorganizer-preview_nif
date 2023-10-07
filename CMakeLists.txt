cmake_minimum_required(VERSION 3.16)

set(DEPENDENCIES_DIR C:/Repositories/bethesda/mo2/build/build)

set(BOOST_ROOT ${DEPENDENCIES_DIR}/boost_1_83_0)
set(QT_ROOT C:/Programs/Qt/6.5.2/msvc2019_64)
set(FMT_ROOT ${DEPENDENCIES_DIR}/fmt-8.1.1)
set(PYTHON_ROOT ${DEPENDENCIES_DIR}/python-3.11.5)
set(CMAKE_INSTALL_PREFIX C:/Repositories/bethesda/mo2/build/install)
set(LOOT_PATH ${DEPENDENCIES_DIR}/libloot-0.22.0-win64)
set(SPDLOG_ROOT ${DEPENDENCIES_DIR}/spdlog-v1.10.0)
set(LIBBSARCH_ROOT ${DEPENDENCIES_DIR}/libbsarch-0.0.9-release-x64)
set(ZLIB_ROOT ${DEPENDENCIES_DIR}/zlib-v1.3)
set(LZ4_ROOT ${DEPENDENCIES_DIR}/lz4-v1.9.4)
set(SEVENZ_ROOT ${DEPENDENCIES_DIR}/7zip-23.01)

if(DEFINED DEPENDENCIES_DIR)
	include(${DEPENDENCIES_DIR}/modorganizer_super/cmake_common/mo2.cmake)
else()
	include(${CMAKE_CURRENT_LIST_DIR}/cmake_common/mo2.cmake)
endif()

project(preview_nif)

add_subdirectory(src)
set(GLI_TEST_ENABLE OFF CACHE BOOL "Build gli unit tests")
add_subdirectory(external/gli)
add_subdirectory(external/nifly)

target_link_libraries(${PROJECT_NAME} PRIVATE nifly gli)
