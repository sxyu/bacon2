cmake_minimum_required( VERSION 2.8 )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project( bacon )

set( CMAKE_CXX_STACK_SIZE "10000000" )
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules" )

if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Release )
endif()

set( PROJ_NAME "bacon" )
set( OUTPUT_NAME "bacon" )

include( CheckCXXCompilerFlag )
CHECK_CXX_COMPILER_FLAG( "-std=c++11" COMPILER_SUPPORTS_CXX11 )
CHECK_CXX_COMPILER_FLAG( "-std=c++0x" COMPILER_SUPPORTS_CXX0X )

if( COMPILER_SUPPORTS_CXX11 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
elseif( COMPILER_SUPPORTS_CXX0X )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x" )
else()
    message( STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler." )
endif()

if ( CMAKE_COMPILER_IS_GNUCXX )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations -O3 -g" )
endif ( CMAKE_COMPILER_IS_GNUCXX )

# Require Git
find_package(Git QUIET)

# Auto init submodules
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Updating submodules...")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# PyBind11
add_subdirectory(3rdparty/pybind11)

# Boost
#set( Boost_USE_STATIC_LIBS ON )
#set( Boost_USE_STATIC ON )

#find_package( Boost REQUIRED COMPONENTS system program_options filesystem )

include_directories(
    ${PYTHON_INCLUDE_DIRS}
    #${Boost_INCLUDE_DIRS}
    ./include
)

set(
  SOURCES
  main.cpp
  session.cpp
  core.cpp
  config.cpp
  strategy.cpp
  util.cpp
)
set(
  HEADERS
  include/session.hpp
  include/strategy.hpp
  include/core.hpp
  include/config.hpp
  include/util.hpp
  include/tinydir.h
)

set( OUTPUT_NAME )
pybind11_add_module( ${PROJ_NAME} ${HEADERS} ${SOURCES} )
#target_include_directories( ${OUTPUT_NAME} PRIVATE ${INCLUDE_DIR} )
#set_target_properties( ${PROJ_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME} )
#target_link_libraries( ${PROJ_NAME} ${DEPENDENCIES} )
