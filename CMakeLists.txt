cmake_minimum_required(VERSION 3.9)

project(chess_engine)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra -Wno-sign-compare -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O2 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops -Ofast -march=native")

set(BUILD_TESTING 0)
set(BUILD_DROGON_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_CTL OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_ORM OFF CACHE BOOL "" FORCE)
set(COZ_PROFILING OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/drogon EXCLUDE_FROM_ALL)
unset(COZ_PROFILIN)
unset(BUILD_ORM)
unset(BUILD_EXAMPLES)
unset(BUILD_CTL)
unset(BUILD_TESTING)

find_package(Boost COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_subdirectory(thirdparty/lc0 EXCLUDE_FROM_ALL)

set(SOURCE_FILES
    src/main.cpp
    src/controller.cpp
    src/random_strategy.cpp
    src/book_strategy.cpp
)

set(LIB_LIST
    ${Boost_LIBRARIES}
    drogon
    lc0
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lc0/src")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_LIST})
