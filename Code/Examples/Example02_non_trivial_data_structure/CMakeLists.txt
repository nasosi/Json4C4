project( Example02_non_trivial_data_structure )

add_executable( Example02_non_trivial_data_structure main.cpp )

target_link_libraries(  Example02_non_trivial_data_structure PUBLIC Json4C4 )

set_target_properties( Example02_non_trivial_data_structure
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)