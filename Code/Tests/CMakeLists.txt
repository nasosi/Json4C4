enable_testing()

set( Json4C4TestsWorkingDirectory ${CMAKE_BINARY_DIR}/TestRun )

add_custom_target( Json4C4TestData
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${Json4C4RootDirectory}/Data ${Json4C4TestsWorkingDirectory}/Data
)

function( Json4C4AddTest testName )

    add_executable( ${testName} ${testName}.cpp TestCheck.h )
    target_link_libraries( ${testName} PRIVATE Json4C4::Json4C4 )
    set_target_properties( ${testName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
    add_dependencies( ${testName} Json4C4TestData )

    if ( ( BUILD_SHARED_LIBS AND WIN32 ) OR Json4C4EnableSanitizers )
        target_compile_definitions( ${testName} PRIVATE JSON4C4_TEST_NO_ALLOCATION_HOOK )
    endif()

    if ( MSVC )
        target_compile_definitions( ${testName} PRIVATE _CRT_SECURE_NO_WARNINGS )
        target_compile_options( ${testName} PRIVATE /W4 /WX )
    else()
        target_compile_options( ${testName} PRIVATE -Wall -Wextra -Werror -Wno-strict-aliasing )
    endif()

    if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
        target_compile_options( ${testName} PRIVATE -Wno-error=stringop-overflow )
    endif()

    add_test( NAME ${testName} COMMAND $<TARGET_FILE:${testName}> WORKING_DIRECTORY ${Json4C4TestsWorkingDirectory} )
    set_tests_properties( ${testName} PROPERTIES SKIP_RETURN_CODE 77 )

endfunction()

Json4C4AddTest( test01 )
Json4C4AddTest( test02 )
Json4C4AddTest( test03 )
Json4C4AddTest( test04 )
Json4C4AddTest( test05 )
Json4C4AddTest( test06 )
Json4C4AddTest( test07 )
Json4C4AddTest( test08 )
Json4C4AddTest( test09 )
Json4C4AddTest( test10 )
Json4C4AddTest( test11 )
Json4C4AddTest( test12 )
Json4C4AddTest( test13 )
Json4C4AddTest( test14 )
Json4C4AddTest( test15 )
Json4C4AddTest( test16 )
Json4C4AddTest( test17 )
