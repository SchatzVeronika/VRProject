#Template for INFOH502
#Heavily inspired from https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

#project setup
project(infoh502-cpp)

#set the minimal version of cmake and the minimal version of the c++ language
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)      
set(CMAKE_VERBOSE_MAKEFILE ON)

#define some variable
set(COMPILE_LAB01 OFF CACHE BOOL "Compile the first Lab")
set(COMPILE_LAB02 OFF CACHE BOOL "Compile the second Lab")
set(COMPILE_LAB03 ON CACHE BOOL "Compile the third Lab")
set(COMPILE_SOLUTIONS ON CACHE BOOL "Compile the solutions")


find_package(OpenGL REQUIRED)

#for glad library
add_library( glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

#for GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

#Search for glfw in case it is installed
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    #if glfw is not found, we will use the sources from the submodules
    add_subdirectory(3rdParty/glfw)
    include_directories(3rdParty/glfw/include/)
endif()



include_directories(3rdParty/glad/include/
                    3rdParty/glm/
                    3rdParty/stb/)

                 


if(COMPILE_LAB03)
	add_subdirectory(LAB03)
endif()
