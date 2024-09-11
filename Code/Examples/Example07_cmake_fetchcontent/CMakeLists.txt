# This example is meant to serves as a reference of how to use CMake's FetchContent_Declare to link to Json4C4 
# when used in standalone (outside the C4-Engine) mode
cmake_minimum_required (VERSION 3.15)

include( FetchContent )

# Uncomment to disable std::vector, std::map, std::string support
#set(Json4C4EnableStdSupport No CACHE INTERNAL "") 

FetchContent_Declare(
  Json4C4
  GIT_REPOSITORY "https://github.com/nasosi/Json4C4"
  GIT_TAG v1.1.1
)

FetchContent_MakeAvailable( Json4C4 )

add_executable (example07_cmake_fetchcontent "main.cpp" )

target_link_libraries( example07_cmake_fetchcontent PRIVATE Json4C4::Json4C4 )
