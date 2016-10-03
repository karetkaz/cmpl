cmake_minimum_required(VERSION 3.3)
project(cimpl)

set(ARCH "64")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m${ARCH}")

set(CC_HEADER_FILES
	src/cimpl.h
	src/vmCore.h
	src/ccCore.h
	src/internal.h
)

set(CC_SOURCE_FILES
	src/cgen.c
	src/code.c
	src/core.c
	src/lexer.c
	src/lstd.c
	src/parser.c
		src/plugin.c
	src/printer.c
	src/tree.c
	src/type.c
)

add_executable(cc
	${CC_SOURCE_FILES}
	src/main.c
)

target_link_libraries(cc m dl)