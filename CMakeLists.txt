cmake_minimum_required (VERSION 3.15.0)
project (dataChecker)


##############################
# Setting build options
##############################
# RELEASE Build Type
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Default build is Release")
  set(CMAKE_BUILD_TYPE "Release")
endif ()

# SUPER BUILD OPTION 
option(USE_SUPERBUILD "Whether or not a superbuild should be invoked" ON)
set( THIRD_PARTY_BUILD_DIR "${PROJECT_SOURCE_DIR}/third_party_build" CACHE PATH "Superbuild build dir")
set( THIRD_PARTY_INSTALL_DIR "${PROJECT_SOURCE_DIR}/third_party" CACHE PATH "Superbuild install dir")

# YAAP 
set(YAAP_INCLUDE_DIR "" CACHE PATH "Yaap include directory")

##############################
# Set additional compiler option
##############################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (USE_SUPERBUILD)
    message(STATUS "Use SUPER_BUILD")
    list(APPEND thirdparties_forward_options
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_INSTALL_MESSAGE=NEVER"
        )

    include (${PROJECT_SOURCE_DIR}/cmake/SuperBuild.cmake)
endif()

include_directories(${YAAP_INCLUDE_DIR})

add_executable(${PROJECT_NAME} src/main.cpp)

if( USE_SUPERBUILD )
    add_dependencies(${PROJECT_NAME} yaap_ep)
endif( USE_SUPERBUILD )
