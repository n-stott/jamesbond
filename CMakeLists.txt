cmake_minimum_required(VERSION 3.23)
project(jamesbond)
set(CMAKE_CXX_STANDARD 17)

link_directories(external)

add_library(jamesbond SHARED
    src/players/qlearner.cpp
    src/players/shapley.cpp
    src/players/bilinear.cpp
    src/bilinearminmax.cpp
    src/gamearena.cpp
    src/gamestate.cpp
    src/capi.cpp
)
target_compile_options(jamesbond PUBLIC -fPIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
target_include_directories(jamesbond PUBLIC include)
target_include_directories(jamesbond PUBLIC external)

add_executable(jamesbond-bin src/main.cpp )
target_compile_options(jamesbond-bin PUBLIC -fPIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
target_include_directories(jamesbond-bin PUBLIC include)
target_include_directories(jamesbond-bin PUBLIC external)
target_link_libraries(jamesbond-bin PUBLIC jamesbond)
target_include_directories(jamesbond PUBLIC external/glpk-5.0/src)
target_link_libraries(jamesbond PUBLIC libglpk.a)
set_target_properties(jamesbond-bin PROPERTIES OUTPUT_NAME jamesbond)

add_executable(test_bilinear_solve src/bilinearsolve.cpp)
target_compile_options(test_bilinear_solve PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
target_include_directories(test_bilinear_solve PUBLIC include)
target_include_directories(test_bilinear_solve PUBLIC external)
target_include_directories(test_bilinear_solve PUBLIC external/glpk-5.0/src)
target_link_libraries(test_bilinear_solve PUBLIC libglpk.a)


# add_executable(reachability_analysis
#     src/reachability_analysis.cpp
# )
# target_compile_options(reachability_analysis PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror -DFMT_HEADER_ONLY)
# target_include_directories(reachability_analysis PUBLIC include)
# target_include_directories(reachability_analysis PUBLIC external)

# add_subdirectory(tests)