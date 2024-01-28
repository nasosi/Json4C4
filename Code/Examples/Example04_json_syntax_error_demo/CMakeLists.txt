project( Example04_json_syntax_error_demo )

add_executable( Example04_json_syntax_error_demo main.cpp )

target_link_libraries(  Example04_json_syntax_error_demo PUBLIC Json4C4 )

set_target_properties( Example04_json_syntax_error_demo
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)