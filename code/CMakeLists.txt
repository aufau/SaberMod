# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

# mv_setup.h preparation
set(MVGLOBALIncludeDirs ${MVGLOBALIncludeDirs} ${CMAKE_BINARY_DIR})

add_subdirectory("game")
add_subdirectory("cgame")
add_subdirectory("ui")

if(BuildQVMs)
	add_subdirectory("tools")

	# Custom target for building qvm's
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vm_build")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vm")

	foreach(FILE ${MVGAMEVMSOURCES})
		file(TO_NATIVE_PATH "${FILE}" FILE_NATIVE)
		set(MVGAMEVMSOURCES_NATIVE ${MVGAMEVMSOURCES_NATIVE} ${FILE_NATIVE})

		get_filename_component(FILE ${FILE} NAME_WE)
		set(MVGAMEASMFILES ${MVGAMEASMFILES} "${FILE}.asm")
	endforeach()

	foreach(FILE ${MVCGAMEVMSOURCES})
		file(TO_NATIVE_PATH "${FILE}" FILE_NATIVE)
		set(MVCGAMEVMSOURCES_NATIVE ${MVCGAMEVMSOURCES_NATIVE} ${FILE_NATIVE})

		get_filename_component(FILE ${FILE} NAME_WE)
		set(MVCGAMEASMFILES ${MVCGAMEASMFILES} "${FILE}.asm")
	endforeach()

	foreach(FILE ${MVUIVMSOURCES})
		file(TO_NATIVE_PATH "${FILE}" FILE_NATIVE)
		set(MVUIVMSOURCES_NATIVE ${MVUIVMSOURCES_NATIVE} ${FILE_NATIVE})

		get_filename_component(FILE ${FILE} NAME_WE)
		set(MVUIASMFILES ${MVUIASMFILES} "${FILE}.asm")
	endforeach()

	file(RELATIVE_PATH MVSDK_CGAME_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/cgame")
	file(TO_NATIVE_PATH "${MVSDK_CGAME_PATH}" MVSDK_CGAME_PATH)
	
	file(RELATIVE_PATH MVSDK_GAME_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/game")
	file(TO_NATIVE_PATH "${MVSDK_GAME_PATH}" MVSDK_GAME_PATH)
	
	file(RELATIVE_PATH MVSDK_UI_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/ui")
	file(TO_NATIVE_PATH "${MVSDK_UI_PATH}" MVSDK_UI_PATH)
	
	file(RELATIVE_PATH MVSDK_BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${CMAKE_BINARY_DIR}")
	file(TO_NATIVE_PATH "${MVSDK_BIN_PATH}" MVSDK_BIN_PATH)
	
	add_custom_target(qvms
		COMMAND $<TARGET_FILE:${LCC}>
			"-DQ3_VM" "-DNDEBUG" "-DGIT_VERSION=\\\"${MVSDK_VERSION}\\\"" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVGAMEVMSOURCES_NATIVE}

		COMMAND $<TARGET_FILE:${ASM}>
			"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm/jk2mpgame.qvm" ${MVGAMEASMFILES} ${MVGAMEASM}

		COMMAND $<TARGET_FILE:${LCC}>
			"-DQ3_VM" "-DNDEBUG" "-DGIT_VERSION=\\\"${MVSDK_VERSION}\\\"" "-DJK2AWARDS" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${MVSDK_CGAME_PATH}" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVCGAMEVMSOURCES_NATIVE}

		COMMAND $<TARGET_FILE:${ASM}>
			"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm/cgame.qvm" ${MVCGAMEASMFILES} ${MVCGAMEASM}

		COMMAND $<TARGET_FILE:${LCC}>
			"-DQ3_VM" "-DNDEBUG" "-DGIT_VERSION=\\\"${MVSDK_VERSION}\\\"" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${SRCDir}/cgame" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVUIVMSOURCES_NATIVE}

		COMMAND $<TARGET_FILE:${ASM}>
			"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm/ui.qvm" ${MVUIASMFILES} ${MVUIASM}

		WORKING_DIRECTORY
			"${CMAKE_CURRENT_BINARY_DIR}/vm_build"

		DEPENDS
			${LCC} ${CPP} ${RCC} ${ASM}
	)

	set_property(TARGET qvms PROPERTY PROJECT_LABEL "Build QVMs")
	if(InMV)
		set_property(TARGET qvms PROPERTY FOLDER "Modules")
	endif()
endif()
