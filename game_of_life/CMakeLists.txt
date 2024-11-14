cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(game_of_life)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -ffast-math -gdwarf-4 -msse -msse2 -mfpmath=sse -march=native -fopenmp")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -Wall -Wextra -O3 -ffast-math -gdwarf-4 -msse -msse2 -mfpmath=sse -march=native -fopenmp")

set(HEADERS game.hpp)
set(SOURCES game.cpp ${HEADERS})
add_executable(game ${SOURCES})
target_link_libraries(game PUBLIC "-lsfml-graphics -lsfml-window -lsfml-system")
# target_include_directories(game PUBLIC include)

