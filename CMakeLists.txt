cmake_minimum_required (VERSION 3.12)

set(MY_VCPKG_DIR $ENV{HOMEDRIVE}$ENV{HOMEPATH}\\vcpkg)

if(NOT EXISTS ${MY_VCPKG_DIR})
  message("Cloning vcpkg in ${MY_VCPKG_DIR}")
  execute_process(COMMAND git clone https://github.com/Microsoft/vcpkg.git ${MY_VCPKG_DIR})
endif()

if(NOT EXISTS ${MY_VCPKG_DIR}\\README.md)
  message(FATAL_ERROR "***** FATAL ERROR: Could not clone vcpkg *****")
endif()

if(NOT EXISTS ${MY_VCPKG_DIR}\\vcpkg.exe)
  message("Bootstrapping vcpkg in ${MY_VCPKG_DIR}")
  execute_process(COMMAND ${MY_VCPKG_DIR}\\bootstrap-vcpkg.bat WORKING_DIRECTORY ${MY_VCPKG_DIR})
endif()

if(NOT EXISTS ${MY_VCPKG_DIR}\\vcpkg.exe)
  message(FATAL_ERROR "***** FATAL ERROR: Could not bootstrap vcpkg *****")
endif()

set(MY_PROJECT_DEPENDENCIES boost-core boost-optional)
message(STATUS "***** Checking project third party dependencies in ${MY_VCPKG_DIR} *****")
execute_process(COMMAND ${MY_VCPKG_DIR}\\vcpkg.exe install ${MY_PROJECT_DEPENDENCIES} WORKING_DIRECTORY ${MY_VCPKG_DIR})
set(CMAKE_TOOLCHAIN_FILE ${MY_VCPKG_DIR}\\scripts\\buildsystems\\vcpkg.cmake)
set(Boost_INCLUDE_DIR ${MY_VCPKG_DIR}\\installed\\x86-windows\\include)
project (CppGenerators)
find_package(Boost 1.67 REQUIRED)

message("***** VCPKG INCLUDE DIRS: ${Boost_INCLUDE_DIR} *****")

include_directories(${PROJECT_SOURCE_DIR}/include ${Boost_INCLUDE_DIR})
add_executable(driver test/driver.cxx)
set_property(TARGET driver PROPERTY CXX_STANDARD 17)
