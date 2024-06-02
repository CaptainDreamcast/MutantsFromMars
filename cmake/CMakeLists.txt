cmake_minimum_required(VERSION 3.25)
project(Mutants CXX)               
set(CMAKE_CXX_STANDARD 17)            
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include("${CMAKE_SOURCE_DIR}/../../addons/prism/cmake/prism.cmake" REQUIRED)

# Add source/header files to project
file(GLOB_RECURSE SOURCES ../*.cpp SOURCES ../*.h SOURCES ../*.rc)
list(FILTER SOURCES EXCLUDE REGEX ".*web.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")

add_link_options(/NODEFAULTLIB:libcmt.lib)
add_link_options(/IGNORE:4099,4286,4098)
add_link_options(/OPT:REF)
add_link_options(/OPT:ICF)

# Define exe
add_executable(Mutants ${SOURCES})

# Link with libraries
target_link_libraries(Mutants prism)
target_link_libraries(Mutants SDL2)
target_link_libraries(Mutants SDL2main)
target_link_libraries(Mutants SDL2_image)
target_link_libraries(Mutants SDL2_mixer)
target_link_libraries(Mutants SDL2_ttf)
target_link_libraries(Mutants enet)
target_link_libraries(Mutants freetype)
target_link_libraries(Mutants glew)
target_link_libraries(Mutants curl)
target_link_libraries(Mutants libjpeg)
target_link_libraries(Mutants libpng)
target_link_libraries(Mutants libwebp)
target_link_libraries(Mutants zstd)
target_link_libraries(Mutants zlib)
target_link_libraries(Mutants crypt32)
target_link_libraries(Mutants wldap32)
target_link_libraries(Mutants ws2_32)
target_link_libraries(Mutants winmm)
target_link_libraries(Mutants OpenGL32)
target_link_libraries(Mutants version)

# Define preprocessor definitions
target_compile_definitions(Mutants PUBLIC UNICODE)
target_compile_definitions(Mutants PUBLIC _UNICODE)

set_property(TARGET Mutants PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(Mutants PRIVATE /Gy)

set_target_properties(Mutants PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../assets)
set_target_properties(Mutants PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../assets)

# Copy over DLLs
file(GLOB DLLS "${CMAKE_SOURCE_DIR}/../../addons/prism/windows/vs17/DLL/*.dll")
foreach(DLL ${DLLS})
  add_custom_command(TARGET Mutants POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${DLL}
    $<TARGET_FILE_DIR:Mutants>)
endforeach()

# Optimize for release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(Mutants PRIVATE /O2)
endif()
