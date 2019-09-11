# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

set(LBURGFiles
	"lburg.h"

	"gram.c"
	"lburg.c"
)

add_executable(${LBURG} ${LBURGFiles})
set_target_properties(${LBURG} PROPERTIES PROJECT_LABEL "lburg")
set_property(TARGET ${LBURG} PROPERTY FOLDER "Tools")
