#=================================================
#
# Noel Lopes is a Professor at the Polytechnic of Guarda, Portugal
# and a Researcher at the CISUC - University of Coimbra, Portugal
# Copyright (C) 2017 Noel de Jesus Mendonça Lopes
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#	http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=================================================

cmake_minimum_required(VERSION 3.0.2)

#=================================================
# Project: cppx
#
# Currnetly cppx features the following tools:
#
# cppxgen: Converts extended C++ files (.cppx) to standard C++ files (.h and .cpp)
#=================================================
project(cppx VERSION 0.0.1.1)

#-------------------------------------------------
# Make sure a C++ 11 compiler is available
#-------------------------------------------------
if (NOT MSVC)
    include(CheckCXXCompilerFlag)

    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_Cx11)

    if (COMPILER_SUPPORTS_Cx11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        message(WARNING "The compiler ${CMAKE_CXX_COMPILER} does not appear to support C++11. Please select a different C++ compiler if that is the case.")
    endif()
endif()
#-------------------------------------------------

#-------------------------------------------------
# Boost
#-------------------------------------------------
set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED   ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.63.0 REQUIRED filesystem)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})   
else()
    message(FATAL_ERROR "Could not find Boost 1.63.0 or greater (http://www.boost.org/).")
endif()
#-------------------------------------------------

#-------------------------------------------------
# cppxgen
# Converts extended C++ files (.cppx) to standard C++ files (.h and .cpp)
#-------------------------------------------------
set (source_files
	src/cppxgen.cpp
	src/console.cpp
) 

add_executable(cppxgen ${source_files})

set_property(
	SOURCE src/cppxgen.cpp
	APPEND PROPERTY COMPILE_DEFINITIONS
	BOOST_FILESYSTEM_NO_DEPRECATED # Tell Boost NOT to use filesystem deprecated names and features
	CPPX_VERSION_STRING="cppxgen version ${PROJECT_VERSION}" 
)

target_link_libraries(cppxgen Boost::filesystem)
