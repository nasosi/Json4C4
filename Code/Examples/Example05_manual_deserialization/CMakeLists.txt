project( Example05_manual_deserialization )

add_executable( Example05_manual_deserialization main.cpp )

target_link_libraries(  Example05_manual_deserialization PUBLIC Json4C4 )

set_target_properties( Example05_manual_deserialization
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)