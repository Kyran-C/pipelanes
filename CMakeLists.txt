cmake_minimum_required( VERSION 3.21 )

project(
  pipelanes
  VERSION 1.0
  LANGUAGES CXX)

set( CMAKE_EXPORT_COMPILE_COMMANDS on )

set( CMAKE_CXX_STANDARD_REQUIRED on )
set( CMAKE_CXX_STANDARD 23 )

add_library( ${PROJECT_NAME}
	INTERFACE
)

target_include_directories( ${PROJECT_NAME}
	INTERFACE
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

get_target_property( INCL_DIRS ${PROJECT_NAME} INTERFACE_INCLUDE_DIRECTORIES )
message( STATUS "Includes for ${PROJECT_NAME}: ${INCL_DIRS}" )


add_executable( ${PROJECT_NAME}_test "tests/main.cpp" )
target_include_directories( ${PROJECT_NAME}_test PRIVATE "${PROJECT_SOURCE_DIR}/include" )

target_compile_options( ${PROJECT_NAME}_test
	PRIVATE
	-fdiagnostics-color=always
)

