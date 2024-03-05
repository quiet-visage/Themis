project(tree_sitter C)

file(GLOB_RECURSE SRCS "src/*.c" "src/*.h" "src/*/.h" "src/*/.c")

add_library(tree_sitter STATIC ${SRCS})
set_property(TARGET tree_sitter PROPERTY C_STANDARD 11)
target_include_directories(tree_sitter PUBLIC "include/")
