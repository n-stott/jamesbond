cmake_minimum_required(VERSION 3.13)
project(jamesbond)
set(CMAKE_CXX_STANDARD 17)


add_library(jamesbond SHARED
    src/players/qlearner.cpp
    src/players/shapley.cpp
    src/gamearena.cpp
    src/gamestate.cpp
    src/capi.cpp
)
target_compile_options(jamesbond PUBLIC -fPIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
target_include_directories(jamesbond PUBLIC include)
target_include_directories(jamesbond PUBLIC external)

add_executable(jamesbond-bin
    src/main.cpp
)
target_compile_options(jamesbond-bin PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
target_include_directories(jamesbond-bin PUBLIC include)
target_include_directories(jamesbond-bin PUBLIC external)
target_link_libraries(jamesbond-bin PUBLIC jamesbond)

set_target_properties(jamesbond-bin PROPERTIES OUTPUT_NAME jamesbond)


# add_executable(reachability_analysis
#     src/reachability_analysis.cpp
# )
# target_compile_options(reachability_analysis PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
# target_include_directories(reachability_analysis PUBLIC include)
# target_include_directories(reachability_analysis PUBLIC external)

add_subdirectory(tests)