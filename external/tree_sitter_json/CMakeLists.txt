project(tree_sitter_json C)

file(GLOB SRCS "*.c")

add_library(tree_sitter_json STATIC ${SRCS})
set_property(TARGET tree_sitter_json PROPERTY C_STANDARD 11)
target_include_directories(tree_sitter_json PUBLIC "tree_sitter/")
