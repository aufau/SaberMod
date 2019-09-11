# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

add_subdirectory("cpp")
add_subdirectory("lburg")
add_subdirectory("etc")
add_subdirectory("src")
