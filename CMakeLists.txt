cmake_minimum_required(VERSION 3.19)
project(las2poly LANGUAGES CXX VERSION 1.1)
add_executable(las2poly src/main.cpp)
include(ExternalProject)
ExternalProject_Add(
	lazperf
	PREFIX ${CMAKE_BINARY_DIR}/lazperf
	GIT_REPOSITORY https://github.com/hobuinc/laz-perf.git
	GIT_TAG 3.4.0
	INSTALL_COMMAND ""
	UPDATE_DISCONNECTED TRUE
)
add_dependencies(las2poly lazperf)
ExternalProject_Get_Property(lazperf SOURCE_DIR)
ExternalProject_Get_Property(lazperf BINARY_DIR)
include_directories(${SOURCE_DIR}/cpp)
target_link_libraries(las2poly ${BINARY_DIR}/cpp/liblazperf_s.a)
target_compile_definitions(las2poly PRIVATE VERSION="${las2poly_VERSION}")
target_compile_options(las2poly PRIVATE -Wfatal-errors -Wall -Wextra -Wpedantic)
target_compile_features(las2poly PRIVATE cxx_std_20)
set_target_properties(las2poly PROPERTIES CXX_EXTENSIONS OFF)
install(TARGETS las2poly RUNTIME)
install(DIRECTORY man1 TYPE MAN)
install(DIRECTORY bash-completions TYPE DATA)
