project(pubsub)

add_library(${PROJECT_NAME}
            pub_sub.cc)

target_link_libraries(${PROJECT_NAME} PUBLIC compiler_flags)
target_link_libraries(${PROJECT_NAME} PUBLIC network)

target_include_directories(${PROJECT_NAME}
                            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
                            )

target_compile_definitions(${PROJECT_NAME} PRIVATE "Debug")