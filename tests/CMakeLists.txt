cmake_minimum_required(VERSION 3.13)
project(jamesbond)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

add_executable(test_capi test_capi.cpp)
target_compile_options(test_capi PUBLIC -fPIC -DFMT_HEADER_ONLY)
target_include_directories(test_capi PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external)
target_link_directories(test_capi PUBLIC ${CMAKE_BINARY_DIR})
target_link_libraries(test_capi PUBLIC jamesbond)
add_test(NAME test_capi COMMAND ${CMAKE_BINARY_DIR}/tests/test_capi)