enable_testing()

set (Json4C4TestsWorkingDirectory ${CMAKE_BINARY_DIR}/bin )

add_executable(test001 test01.cpp)
target_link_libraries(test001 PRIVATE Json4C4::Json4C4)
set_target_properties( test001 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
add_test( NAME ctest01 COMMAND $<TARGET_FILE:test001> WORKING_DIRECTORY ${Json4C4TestsWorkingDirectory} )

add_executable(test002 test02.cpp)
target_link_libraries(test002 PRIVATE Json4C4::Json4C4)
set_target_properties( test002 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
add_test( NAME ctest002 COMMAND $<TARGET_FILE:test002> WORKING_DIRECTORY ${Json4C4TestsWorkingDirectory} )

add_executable(test003 test03.cpp)
target_link_libraries(test003 PRIVATE Json4C4::Json4C4)
set_target_properties( test003 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
add_test( NAME ctest003 COMMAND $<TARGET_FILE:test003> WORKING_DIRECTORY ${Json4C4TestsWorkingDirectory} )

add_executable(test004 test04.cpp)
target_link_libraries(test004 PRIVATE Json4C4::Json4C4)
set_target_properties( test004 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
add_test( NAME ctest004 COMMAND $<TARGET_FILE:test004> WORKING_DIRECTORY ${Json4C4TestsWorkingDirectory} )

add_executable(test005 test05.cpp)
target_link_libraries(test005 PRIVATE Json4C4::Json4C4)
set_target_properties( test005 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
add_test( NAME ctest005 COMMAND $<TARGET_FILE:test005> WORKING_DIRECTORY ${Json4C4TestsWorkingDirectory} )