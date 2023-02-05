cmake_minimum_required(VERSION 3.13)
project(jamesbond)
set(CMAKE_CXX_STANDARD 17)


add_executable(jamesbond
    src/players/qlearner.cpp
    src/gamearena.cpp
    src/gamestate.cpp
    src/main.cpp
)
target_compile_options(jamesbond PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
target_include_directories(jamesbond PUBLIC include)
target_include_directories(jamesbond PUBLIC external)



add_executable(reachability_analysis
    src/reachability_analysis.cpp
)
target_compile_options(reachability_analysis PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
target_include_directories(reachability_analysis PUBLIC include)
target_include_directories(reachability_analysis PUBLIC external)