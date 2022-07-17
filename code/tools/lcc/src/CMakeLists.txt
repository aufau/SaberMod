# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

set(RCCFiles
	"c.h"
	"config.h"
	"token.h"

	"alloc.c"
	"bytecode.c"
	"decl.c"
	"error.c"
	"expr.c"
	"init.c"
	"input.c"
	"list.c"
	"null.c"
	"prof.c"
	"simp.c"
	"string.c"
	"symbolic.c"
	"tree.c"
	"bind.c"
	"dag.c"
	"enode.c"
	"event.c"
	"gen.c"
	"inits.c"
	"lex.c"
	"main.c"
	"output.c"
	"profio.c"
	"stmt.c"
	"sym.c"
	"trace.c"
	"types.c"

	"${CMAKE_CURRENT_BINARY_DIR}/dagcheck.c"
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dagcheck.c"
	COMMAND ${LBURG} "${CMAKE_CURRENT_SOURCE_DIR}/dagcheck.md" "${CMAKE_CURRENT_BINARY_DIR}/dagcheck.c"
	DEPENDS ${LBURG} "${CMAKE_CURRENT_SOURCE_DIR}/dagcheck.md"
)

set(RCCIncludeDirs "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(${RCC} ${RCCFiles})
set_target_properties(${RCC} PROPERTIES INCLUDE_DIRECTORIES "${RCCIncludeDirs}")
set_target_properties(${RCC} PROPERTIES PROJECT_LABEL "q3rcc")
set_property(TARGET ${RCC} PROPERTY FOLDER "Tools")
