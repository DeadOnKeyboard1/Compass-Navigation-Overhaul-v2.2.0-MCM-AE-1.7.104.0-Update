cmake_minimum_required(VERSION 3.21)

########################################################################################################################
## Define project
########################################################################################################################

project(
	CompassNavigationOverhaul
	VERSION 2.2.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	@ONLY
)

file(GLOB_RECURSE headers "include/*.h")
file(GLOB_RECURSE sources "source/*.cpp")

source_group(
	TREE
		${CMAKE_CURRENT_SOURCE_DIR}
	FILES
		${headers}
		${sources}
)

########################################################################################################################
## Configure target DLL
########################################################################################################################

# CommonLibSSE 7.5.4 exports Microsoft::DirectXTK in its public link interface,
# while CNO directly includes Xbyak for the compatibility trampolines. Resolve
# both explicitly so a clean vcpkg build does not depend on accidental
# transitive include/target visibility.
find_package(directxtk CONFIG REQUIRED)
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(spdlog REQUIRED CONFIG)
find_path(XBYAK_INCLUDE_DIRS NAMES xbyak/xbyak.h REQUIRED)

add_commonlibsse_plugin(
	${PROJECT_NAME}
	STRUCT_DEPENDENT
	COMPATIBLE_RUNTIMES 1.7.104.0
	MINIMUM_SKSE_VERSION 2.3.1
	SOURCES ${headers} ${sources}
)
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

target_include_directories(
	${PROJECT_NAME}
	BEFORE PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${XBYAK_INCLUDE_DIRS}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
		spdlog::spdlog_header_only
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		NOMINMAX
)

target_compile_options(
	${PROJECT_NAME}
	PRIVATE
		/Zi
)

if (MSVC)
	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			/DEBUG:FULL
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF>"
	)
endif()

target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
		include/pch.h
)

