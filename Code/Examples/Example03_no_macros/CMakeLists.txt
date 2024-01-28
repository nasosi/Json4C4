project( Example03_no_macros )

add_executable( Example03_no_macros main.cpp )

target_link_libraries(  Example03_no_macros PUBLIC Json4C4 )

set_target_properties( Example03_no_macros
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)