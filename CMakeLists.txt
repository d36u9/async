#/////////////////////////////////////////////////////////////////////
#//          Copyright Yibo Zhu 2017
#// Distributed under the Boost Software License, Version 1.0.
#//    (See accompanying file LICENSE_1_0.txt or copy at
#//          http://www.boost.org/LICENSE_1_0.txt)
#/////////////////////////////////////////////////////////////////////

cmake_minimum_required(VERSION 3.5)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

project(async VERSION 1.0.0 LANGUAGES CXX)


set(BUILD_TESTS ON CACHE BOOL "Build tests.")

#C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

#set up paths
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(BUILD_TESTS)
    option(WITH_BOOST "Build Boost Test" ON)
    option(WITH_CPPRESTSDK "Build Cpprestsdk Test" OFF)
    #option(WITH_TBB "Build TBB Test" OFF) #to be added later...
    if(WITH_BOOST)
    add_subdirectory(thirdparty/boost-cmake)
    add_definitions(-DTEST_BOOST)
    endif()
    if(WITH_CPPRESTSDK)
    add_definitions(-DTEST_PPL)
    endif()
    #if(WITH_TBB)
    #add_definitions(-DTEST_TBB)
    #endif()
endif()

#check thread library
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  add_compile_options(-pthread)
endif()
if(CMAKE_THREAD_LIBS_INIT)
  link_libraries(${CMAKE_THREAD_LIBS_INIT})
endif()



set(LIB_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/async")

include_directories(thirdparty/Catch)
include_directories(thirdparty/rlutil)
include_directories(async)


add_library(libasync INTERFACE)
#target_sources(libasync INTERFACE utility.h queue.h threadpool.h)
target_include_directories(libasync INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/async>
    $<INSTALL_INTERFACE:${LIBRARY_OUTPUT_PATH}/include/async>)

set(LibAsyncHeader ${PROJECT_SOURCE_DIR}/async/utility.h ${PROJECT_SOURCE_DIR}/async/queue.h ${PROJECT_SOURCE_DIR}/async/threadpool.h)


#add to IDE
#add_custom_target(libasync SOURCES ${LibAsyncHeader})

install(FILES "async/*.h"
    DESTINATION "${LIBRARY_OUTPUT_PATH}/include/async")

add_executable(threadpool_bm ${PROJECT_SOURCE_DIR}/test/threadpool_bm.cpp 
    ${LibAsyncHeader}
)

target_link_libraries(threadpool_bm PUBLIC Boost::thread)
target_link_libraries(threadpool_bm PUBLIC Boost::boost)

if(UNIX)
    if(WITH_CPPRESTSDK)
        find_path(CPPREST_INCLUDE cpprest/http_client.h)
        find_library(CPPREST_LIB NAMES cpprest_2_9d cpprest_2_9 cpprestd cpprest)
        find_package(Boost REQUIRED COMPONENTS random system thread filesystem chrono atomic date_time regex)
        find_package(OpenSSL 1.0.0 REQUIRED)

        target_link_libraries(threadpool_bm PUBLIC ${CPPREST_LIB}
        Boost::boost
        Boost::random
        Boost::system
        Boost::thread
        Boost::filesystem
        Boost::chrono
        Boost::atomic
        Boost::date_time
        Boost::regex
        OpenSSL::SSL
        )
    endif()
endif()

add_executable(queue_bm ${PROJECT_SOURCE_DIR}/test/queue_bm.cpp 
    ${LibAsyncHeader}
)

target_link_libraries(queue_bm PUBLIC Boost::boost)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()



