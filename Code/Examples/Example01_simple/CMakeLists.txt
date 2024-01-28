project( example01_simple )

add_executable( example01_simple main.cpp )

target_link_libraries(  example01_simple PUBLIC Json4C4 )

set_target_properties( example01_simple
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


 add_custom_command(
        TARGET example01_simple POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${Json4C4RootDirectory}/Data
            ${CMAKE_BINARY_DIR}/bin/Data
    )