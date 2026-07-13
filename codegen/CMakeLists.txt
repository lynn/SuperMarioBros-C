cmake_minimum_required(VERSION 3.5)

find_package(BISON)
find_package(FLEX)

BISON_TARGET(parser asm6502.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
FLEX_TARGET(scanner asm6502.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(scanner parser)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(codegen
    ${BISON_parser_OUTPUTS}
    ${FLEX_scanner_OUTPUTS}
    ast.cpp 
    translator.cpp
    util.cpp
)

# Note that the translated sources in source/SMB are not regenerated as part of
# the build, and that running this tool over them is not a no-op.
#
# The game reads a good deal of its own code as if it were data, by indexing its
# data tables out of bounds, and what it finds when it does depends on where in
# the ROM everything is stored. The translated code cannot be given that layout,
# so the tables the game reads past have been moved by hand and spelled out with
# the bytes the ROM has around them. Those corrections live in the translated
# sources, and this tool does not know about them, so regenerating would silently
# undo them and desynchronize the game from the console again.
#
# To regenerate anyway, run:
#
#     codegen docs/smbdis.asm .
#
# from the root of the repository, and expect to reapply the corrections. They
# are the blocks in SMBData.cpp whose comments explain what the ROM stores around
# a table, together with the addresses they are given in SMBDataPointers.hpp, and
# the calls to SMBEngine::jumpEngine() in SMB.cpp.
