cmake_minimum_required(VERSION 3.9)

project(lc0)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra -Wno-sign-compare -fno-omit-frame-pointer -fopenmp")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O2 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops -Ofast -march=native")

set(SOURCE_FILES
    src/chess/bitboard.cc
    src/chess/board.cc
    src/utils/logging.cc
    src/chess/position.cc
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE NO_PEXT)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
