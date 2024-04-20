project(network)

add_library(${PROJECT_NAME}
            buffer.cc
            channel.cc
            demultiplex.cc
            eventloop.cc
            stream_socket.cc
            tcp_connection.cc
            tcp_server.cc
            )  

target_link_libraries(${PROJECT_NAME} PUBLIC compiler_flags )
target_link_libraries(${PROJECT_NAME} PUBLIC pubsub)
target_link_libraries(${PROJECT_NAME} PUBLIC proxy)

target_include_directories(${PROJECT_NAME}
                            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
                            )

target_compile_definitions(${PROJECT_NAME} PRIVATE "Debug")