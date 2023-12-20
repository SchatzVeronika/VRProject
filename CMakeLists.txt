
#project setup
project(VRProject)

#set the minimal version of cmake and the minimal version of the c++ language
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)      
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenGL REQUIRED)

#for glad library
add_library( glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

#for bullet library
add_subdirectory("3rdParty/bullet3-3.25")

#set(BULLET_SOURCE_DIR "3rdParty/bullet3-3.25/src")
#file(GLOB_RECURSE BULLET_SOURCE_FILES ${BULLET_SOURCE_DIR}/*.cpp)
#add_library(bullet3-3.25 STATIC ${BULLET_SOURCE_FILES})
#target_include_directories(bullet3-3.25 PUBLIC ${BULLET_SOURCE_DIR})
#target_include_directories(bullet3-3.25 PRIVATE ${OpenCL_INCLUDE_DIRS})
#include_directories("3rdParty/bullet3-3.25/src")

#add_subdirectory("3rdParty/bullet3-3.25")

#set(CMAKE_CXX_STANDARD 17)
#add_executable(VRProject "main.cpp")
#target_include_directories(VRProject PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
#target_link_libraries(VRProject PRIVATE bullet3-3.25)

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

add_subdirectory(Core)