project(tree_sitter_c C)

file(GLOB SRCS "*.c")

add_library(tree_sitter_c STATIC ${SRCS})
set_property(TARGET tree_sitter_c PROPERTY C_STANDARD 11)
target_include_directories(tree_sitter_c PUBLIC "tree_sitter/")
