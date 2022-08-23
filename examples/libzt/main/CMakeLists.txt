
set(PROJ_DIR ${PROJECT_SOURCE_DIR})
idf_component_register(SRCS "./pingable-node.c"
                    INCLUDE_DIRS ".")


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libzt)


    

#target_link_libraries(main PUBLIC libzt)