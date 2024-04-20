cmake_minimum_required(VERSION 3.20)

project(rproxy)

# replace with interface library
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED True)
add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_11)
target_compile_options(compiler_flags INTERFACE -g)
target_compile_options(compiler_flags INTERFACE -D_GLIBCXX_DEBUG)
target_compile_options(compiler_flags INTERFACE -Wall)
# target_compile_options(compiler_flags INTERFACE -Werror)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)


add_executable(${PROJECT_NAME} src/main.cc)

target_link_libraries(${PROJECT_NAME} PRIVATE network)
target_link_libraries(${PROJECT_NAME} PRIVATE proxy)
target_link_libraries(${PROJECT_NAME} PRIVATE pubsub)

target_include_directories(${PROJECT_NAME} PUBLIC
                            "${CMAKE_CURRENT_BINARY_DIR}"
                            )                           