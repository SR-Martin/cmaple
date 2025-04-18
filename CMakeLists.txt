##################################################################
# CMAPLE cmake build definition
# Copyright (c) 2022 Nhan Ly-Trong, Chris Bielow, Nicola De Maio, Bui Quang Minh
##################################################################

# Windows example usages:
#------------------------
#
# To compile a static version:
# cmake -DCMAPLE_FLAGS=static <source_dir>
#
# cmake -G "Visual Studio 12" <source_dir>          (32-bit version, compiled with MSVC)
# cmake -G "Visual Studio 12 Win64" <source_dir>    (64-bit version, compiled with MSVC)
## using clang.exe as shipped with Visual Studio:
# cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G "Ninja" -DCMAPLE_FLAGS=static ..\cmaple

# Linux example usages:
#----------------------
#
# cmake <source_dir>
#
# To compile a static version:
# cmake -DCMAPLE_FLAGS=static <source_dir>
#
# To compile with GCC on Linux:
# cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ <source_dir>
#
# To compile with CLANG on Linux:
# cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ <source_dir>
#


# Mac OSX example usages:
#------------------------
#
# cmake <source_dir>
#


# Xcode project example usages:
#------------------------------
#
# To generate Xcode project:
# cmake -G Xcode <source_dir>
#


# Disable compilation of googlemock and installation of Googletest
#------------------------------
#
# cmake -DBUILD_GMOCK=OFF -DINSTALL_GTEST=OFF <source_dir>
#


cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

set(GCC_MIN_VERSION "5")


##############################################################
# Clang-tidy
##############################################################
if (USE_CLANG_TIDY)
  message("Setup clang-tidy (USE_CLANG_TIDY=ON)")
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.;-checks=performance-*")
else()
  message("No clang-tidy (USE_CLANG_TIDY=OFF)")
endif()


project(cmaple)
add_definitions(-DCMAPLE)
set(CMAKE_CXX_STANDARD 20)

# Add policy to avoid warnings
cmake_policy(SET CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

if (((${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch") AND (NOT APPLE AND UNIX))
    OR WIN32)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE) # Disable IPO (LTO) for Linux ARM
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) # Enable IPO (LTO) by default
endif()

# The version number.
set (cmaple_VERSION_MAJOR 1)
set (cmaple_VERSION_MINOR 1)
set (cmaple_VERSION_PATCH ".0")

option(BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
option(INSTALL_CMAPLE "Enable installation of CMAPLE. (Projects integrating CMAPLE may want to turn this OFF.)" ON)

##################################################################
# check existence of a few basic functions and headers
##################################################################
include (CheckFunctionExists)
check_function_exists (gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists (getrusage HAVE_GETRUSAGE)
check_function_exists (GlobalMemoryStatusEx HAVE_GLOBALMEMORYSTATUSEX)
check_function_exists (strndup HAVE_STRNDUP)
check_function_exists (strtok_r HAVE_STRTOK_R)
## find optional headers
include (CheckIncludeFile)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTDH)



if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Build mode   : Release")
endif()

if (CMAKE_GENERATOR MATCHES "Xcode")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif()

include_directories("${PROJECT_SOURCE_DIR}")
include_directories(after system "/usr/local/include")

##################################################################
# Build as static library
##################################################################
if (WIN32)
    message("Target OS     : Windows")
    # build as static binary to run on most machines
    if (CMAPLE_FLAGS MATCHES "static")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
elseif (APPLE)
    message("Target OS     : Mac OS X")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
elseif (UNIX)
    message("Target OS     : Unix")
    # build as static binary to run on most machines
    if (NOT CMAPLE_FLAGS MATCHES "static")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
else()
    message("Target OS     : Unknown and untested yet")
endif()

# NHANLT: Note: to build a static version, we have to link
# the static OpenMP library manually here instead of the dynamic one
##################################################################
# configure OpenMP to build a static version
##################################################################
if (NOT CMAPLE_FLAGS MATCHES "single")
    message("OpenMP        : Yes")

    if(APPLE)
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch")
            link_directories(${PROJECT_SOURCE_DIR}/libraries/static/libmac_m1)
        else()
            link_directories(${PROJECT_SOURCE_DIR}/libraries/static/libmac)
        endif()
    elseif (UNIX AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch")
        link_directories(${PROJECT_SOURCE_DIR}/libraries/static/liblinux_arm)
      else()
        link_directories(${PROJECT_SOURCE_DIR}/libraries/static/lib)
      endif()
    elseif (WIN32)
        if (BINARY32)
            link_directories(${PROJECT_SOURCE_DIR}/libraries/static/lib32)
        else()
            link_directories(${PROJECT_SOURCE_DIR}/libraries/static/lib)
        endif()
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if (APPLE OR WIN32)
            if (WIN32)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -openmp -pthread")
            else()
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp -pthread")
            endif()
            if(NOT "${CMAKE_EXE_LINKER_FLAGS}" MATCHES "-lomp")
    			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lomp")
			endif()
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
        endif()
    endif()
else()
    message("OpenMP        : NONE")
endif()

##################################################################
# Setup compiler flags
##################################################################

## enable 'SSE/AVX' on x86-64, 'neon' on arm to achive faster computations (mainly the Matrix::dotProduct())
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")
    add_compile_options(-msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx) # needed for simde instructions
  elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch")
    # NHANLT: Because the option "-neon" is not found,
    # I changed it to "-march=native"
    #add_compile_options(-neon)
    if (NOT APPLE AND UNIX)
        add_compile_options(-march=armv8-a)
    else()
        add_compile_options(-march=native)
    endif()
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")
    add_compile_options(/arch:AVX2) ## not really needed, but may be beneficial in other places when in Release mode (i.e. with optimizations turned on)
  endif()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    message("C flags       : ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
    message("CXX flags     : ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("C flags       : ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    message("CXX flags     : ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

message("LINKER flags  : ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

find_package(Backtrace)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/cmaple_config.h.in"
  "${PROJECT_BINARY_DIR}/cmaple_config.h"
  )

# add the binary tree to the search path for include files
# so that we will find cmaple_config.h
include_directories("${PROJECT_BINARY_DIR}")

#zlib will be detected for appearance
#include_directories("${PROJECT_BINARY_DIR}/zlib-1.2.7")


if (NOT CMAPLE_FLAGS MATCHES "nozlib" AND NOT CMAPLE_FLAGS MATCHES "static")
    find_package(ZLIB)
endif()

if(ZLIB_FOUND)
  message ("Using system zlib")
  include_directories(${ZLIB_INCLUDE_DIRS})
else(ZLIB_FOUND)
  message ("Using own zlib-1.2.7")
  include_directories("${PROJECT_BINARY_DIR}/libraries/zlib-1.2.7" "${PROJECT_SOURCE_DIR}/libraries/zlib-1.2.7")
  if (NOT TARGET zlibstatic)
    add_subdirectory(libraries/zlib-1.2.7)
  endif()
endif(ZLIB_FOUND)

##################################################################
# fetch GoogleTest framework when CMake runs
##################################################################
SET(BUILD_GMOCK OFF CACHE BOOL "Disable compilation of googlemock" FORCE)
SET(INSTALL_GTEST OFF CACHE BOOL "Disable installation of googletest" FORCE)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

##################################################################
# subdirectories containing necessary libraries for the build
##################################################################
LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")
if (NOT TARGET ncl)
    add_subdirectory(libraries/ncl)
endif()
if (NOT TARGET nclextra)
    add_subdirectory(libraries/nclextra)
endif()
add_subdirectory(utils)
add_subdirectory(libraries/SIMDe)
target_include_directories(cmaple_utils PUBLIC ${SIMDE_INCLUDE_DIRECTORY})
add_subdirectory(model)
add_subdirectory(alignment)
add_subdirectory(tree)
add_subdirectory(maple)
add_subdirectory(unittest)


##################################################################
# the main executable
##################################################################
add_executable(cmaple main/main.cpp)
if (USE_CMAPLE_AA)
    add_executable(cmaple-aa main/main.cpp)
endif()

if(Backtrace_FOUND)
  include_directories(${Backtrace_INCLUDE_DIR})
  target_link_libraries(cmaple PRIVATE ${Backtrace_LIBRARY})
    if (USE_CMAPLE_AA)
      target_link_libraries(cmaple-aa PRIVATE ${Backtrace_LIBRARY})
    endif()
endif(Backtrace_FOUND)

##################################################################
# setup linking flags
##################################################################

target_link_libraries(cmaple PUBLIC maple cmaple_tree cmaple_model ncl nclextra)
if (USE_CMAPLE_AA)
    target_link_libraries(cmaple-aa PUBLIC maple-aa cmaple_tree-aa cmaple_model-aa ncl nclextra)
endif()

##############################################################
# add the install targets
##############################################################
if (INSTALL_CMAPLE)
    install (TARGETS cmaple DESTINATION bin)
    if (USE_CMAPLE_AA)
        install (TARGETS cmaple-aa DESTINATION bin)
    endif()
    install (FILES "${PROJECT_SOURCE_DIR}/example/example.maple" DESTINATION .)
    install (FILES "${PROJECT_SOURCE_DIR}/example/tree.nwk" DESTINATION .)
    #install (FILES "${PROJECT_BINARY_DIR}/cmaple_config.h" DESTINATION "include")
    #install(DIRECTORY "${PROJECT_SOURCE_DIR}/" # source directory
    #        DESTINATION "include/cmaple" # target directory
    #        FILES_MATCHING # install only matched files
    #        PATTERN "*.h" # select header files
    #)
endif()

##############################################################
# build a CPack driven installer package
##############################################################
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION_MAJOR "${cmaple_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${cmaple_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${cmaple_VERSION_PATCH}")
if(WIN32 OR APPLE)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_SOURCE_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_SOURCE_GENERATOR "TGZ")
endif()

#set(CPACK_SOURCE_PACKAGE_FILE_NAME
#  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build.*/;/debug.*/;/examples/;/test_scripts/;/manual/;/.bzr/;~$;/\\\\.svn/;/\\\\.git/;/pllrepo/;${CPACK_SOURCE_IGNORE_FILES}")

set (SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if (IQTREE_FLAGS MATCHES "oldmac")
        set (SYSTEM_NAME "macOS10.5")
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch")
        set (SYSTEM_NAME "macOS-arm")
    else()
        set (SYSTEM_NAME "macOS-intel")
    endif()
endif()
if (NOT APPLE AND UNIX)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch")
        set (SYSTEM_NAME "Linux-arm")
    else()
        set (SYSTEM_NAME "Linux-intel")
    endif()
endif()

set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}${EXE_SUFFIX}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}${CPACK_PACKAGE_VERSION_PATCH}-${SYSTEM_NAME}")

if (NOT APPLE)
    set(CPACK_STRIP_FILES TRUE)
endif()

include (CPack)

#add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
