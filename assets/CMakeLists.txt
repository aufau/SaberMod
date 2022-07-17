# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

include(InstallZIP)

# generate assets.pk3
if(MultiConfigGenerator)
	set(OUTFILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/base/assets.pk3)
else()
	set(OUTFILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/base/assets.pk3)
endif()

file(GLOB_RECURSE MVASSETS
	LIST_DIRECTORIES FALSE
	RELATIVE "${CMAKE_SOURCE_DIR}/assets"
	"*")

include(InstallZIP)
add_zip_command(${CMAKE_CURRENT_BINARY_DIR}/assets.pk3 ${OUTFILE}
	DIR "${CMAKE_SOURCE_DIR}/assets"
	FILES "${MVASSETS}"
)

add_custom_target(assets DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/assets.pk3" ${AssetsFiles})
