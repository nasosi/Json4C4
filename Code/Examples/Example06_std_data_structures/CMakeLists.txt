project( example01_simple )

add_executable( Example06_std_data_structures main.cpp )

target_link_libraries(  Example06_std_data_structures PUBLIC Json4C4 )

set_target_properties( Example06_std_data_structures
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)