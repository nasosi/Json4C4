# Json4C4
# A C++ JSON library
#
# MIT License
#
# Copyright(c) 2024 Athanasios Iliopoulos
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# This software uses third party libraries. Their licenses can be found in the
# "Licenses" subfolder of the source code repository.

cmake_minimum_required( VERSION 3.15 )

file(READ "Code/Json4C4/C4Json.cpp" C4JsonCpp)

string(REGEX MATCH "JSON4C4VERSIONMAJOR ([0-9]*)" _ ${C4JsonCpp})
set(Json4C4VersionMajor ${CMAKE_MATCH_1})
string(REGEX MATCH "JSON4C4VERSIONMINOR ([0-9]*)" _ ${C4JsonCpp})
set(Json4C4VersionMinor ${CMAKE_MATCH_1})
string(REGEX MATCH "JSON4C4VERSIONPATCH ([0-9]*)" _ ${C4JsonCpp})
set(Json4C4VersionPatch ${CMAKE_MATCH_1})

set( Json4C4Version ${Json4C4VersionMajor}.${Json4C4VersionMinor}.${Json4C4VersionPatch} )

if( NOT DEFINED PROJECT_NAME )

    set( Json4C4NotSubProject ON )

else()

    set( Json4C4NotSubProject OFF )

endif()


project( Json4C4 VERSION ${Json4C4Version} )

set( Json4C4VDirWithVersion Json4C4-${Json4C4Version} )

message( STATUS "Json4C4: version: " ${Json4C4Version} )

option(Json4C4EnableStdSupport "Enable std::string and std::vector support" Yes)
option(Json4C4DisableExceptions "Disable Exceptions" No)


If ( ${Json4C4DisableExceptions} )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")

            string(REGEX REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

        elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")

            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

      endif()

    elseif( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )

        string(REGEX REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

    endif()

    message( STATUS "Json4C4: exceptions disabled")

endif()


set( Json4C4Src
  Code/Json4C4/C4Json.cpp
  Code/Terathon/TSMap.cpp
  Code/Terathon/TSMath.cpp
  Code/Terathon/TSString.cpp
  Code/Terathon/TSText.cpp
)

set( Json4C4PublicHeaders
    Code/Json4C4/C4Json.h
    Code/Terathon/TSArray.h
    Code/Terathon/TSBasic.h
    Code/Terathon/TSMap.h
    Code/Terathon/TSPlatform.h
    Code/Terathon/TSString.h
    Code/Terathon/TSText.h
)

add_library( Json4C4 ${Json4C4Src} ${Json4C4PublicHeaders} )
add_library( Json4C4::Json4C4 ALIAS Json4C4 )

IF ( BUILD_SHARED_LIBS )
    message( STATUS "Json4C4: Compiling as dynamic library" )
    target_compile_definitions( Json4C4 PRIVATE TERATHON_EXPORT )
endif()

target_compile_features( Json4C4 PUBLIC cxx_std_17 )

set_target_properties( Json4C4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

if(MSVC)

  target_compile_options(Json4C4 PRIVATE /W4 /WX)

else()

  target_compile_options(Json4C4 PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-strict-aliasing -Wno-uninitialized-const-reference)

endif()


If ( NOT ${Json4C4EnableStdSupport} )

    target_compile_definitions(Json4C4 PUBLIC JSON4C4_DISABLE_STD_SUPPORT)
    message( STATUS "Json4C4: Force disable std:: support")

endif()

target_include_directories( Json4C4
    PUBLIC
        $<INSTALL_INTERFACE:include/${Json4C4VDirWithVersion}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Code>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Code/Terathon>
)


include( GNUInstallDirs )

install( TARGETS Json4C4
    EXPORT Json4C4Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${Json4C4VDirWithVersion}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${Json4C4VDirWithVersion}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include( CMakePackageConfigHelpers )

set( JsonC4CConfigIn Json4C4Config.cmake.in)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${JsonC4CConfigIn}
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/Json4C4Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${Json4C4VDirWithVersion}
)

write_basic_package_version_file(
     ${CMAKE_CURRENT_BINARY_DIR}/cmake/Json4C4ConfigVersion.cmake
     VERSION ${Json4C4Version}
     COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${Json4C4PublicHeaders}
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/${Json4C4VDirWithVersion}/Json4C4
)

install(EXPORT Json4C4Targets
  FILE
    Json4C4Targets.cmake
  NAMESPACE
    JsoN4C4::
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${Json4C4VDirWithVersion}
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/Json4C4Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/Json4C4ConfigVersion.cmake
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${Json4C4VDirWithVersion}
  )


if ( Json4C4NotSubProject )

    set( Json4C4RootDirectory ${CMAKE_CURRENT_LIST_DIR} )

    add_subdirectory( Code/Examples )
    
    enable_testing()   
    add_subdirectory( Code/Tests )

endif( Json4C4NotSubProject )