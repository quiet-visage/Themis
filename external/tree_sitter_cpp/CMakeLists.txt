project(tree_sitter_cpp C)

file(GLOB SRCS "*.c")

add_library(tree_sitter_cpp STATIC ${SRCS})
set_property(TARGET tree_sitter_cpp PROPERTY C_STANDARD 11)
target_include_directories(tree_sitter_cpp PUBLIC "tree_sitter/")
