project(proxy)

add_library(${PROJECT_NAME}
            proxy_server.cc
            loader.cc
            )

target_link_libraries(${PROJECT_NAME} PUBLIC compiler_flags )
target_link_libraries(${PROJECT_NAME} PUBLIC pubsub)
target_link_libraries(${PROJECT_NAME} PUBLIC network)

target_include_directories(${PROJECT_NAME}
                            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
                            )

target_compile_definitions(${PROJECT_NAME} PRIVATE "MSG_ATTACH")