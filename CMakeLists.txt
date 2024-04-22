cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_COMPILER gcc)

project(themis C)

set(CMAKE_C_FLAGS_SANITIZE
    ${CMAKE_C_FLAGS_DEBUG}
    CACHE STRING "Flags used by the C compiler during sanitize builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_SANITIZE
    ${CMAKE_EXE_LINKER_FLAGS_DEBUG}
    CACHE STRING "Flags used for linking binaries during sanitize builds."
          FORCE)
set(CMAKE_C_FLAGS_PROFILE
    "-O2 -pg"
    CACHE STRING "Flags used by the C compiler during sanitize builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE
    "-pg"
    CACHE STRING "Flags used for linking binaries during sanitize builds."
          FORCE)

message(
  "* build type options are : None Debug Release RelWithDebInfo MinSizeRel Sanitize Profile"
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

message("* build type is : ${CMAKE_BUILD_TYPE}")
file(GLOB_RECURSE SRCS "src/*.c" "src/*.h" "src/*/*.h" "src/*/*.c")
add_executable(themis ${SRCS})

if(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
  target_compile_options(themis PRIVATE -O0 -g3 -fsanitize=address)
  target_link_options(themis PRIVATE -fsanitize=address)
endif()

set_property(TARGET themis PROPERTY C_STANDARD 11)
# target_compile_options(themis PRIVATE -Werror)
target_compile_options(themis PRIVATE -Wall -Werror=unused-function)
target_link_options(themis PRIVATE)

add_subdirectory(external/field_fusion)
target_link_libraries(themis PRIVATE field_fusion -lmagic)

set(PLATFORM Desktop)
set(GRAPHICS GRAPHICS_API_OPENGL_43)
option(BUILD_EXAMPLES OFF)
option(CUSTOMIZE_BUILD ON)
option(SUPPORT_FILEFORMAT_SVG ON)
add_subdirectory(external/raylib-5.0)

target_link_libraries(themis PRIVATE raylib)

add_subdirectory(external/tree_sitter)
add_subdirectory(external/tree_sitter_c)
add_subdirectory(external/tree_sitter_cpp)
add_subdirectory(external/tree_sitter_json)

target_link_libraries(themis PRIVATE tree_sitter tree_sitter_c tree_sitter_cpp
                                     tree_sitter_json)

target_include_directories(themis PRIVATE external/subprocess)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ DESTINATION shaders)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION resources)
