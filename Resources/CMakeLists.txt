
if(RESDIR_DEFINED)
	add_custom_target(copy_resources ALL COMMENT "Copying resources")
	install(FILES
		${RESDIR}/pak000-Base.pak
		${RESDIR}/pak001-Sounds.pak
		${RESDIR}/pak002-Models.pak
		${RESDIR}/pak010-BaseSkin.pak
		${RESDIR}/pak999-References.pak
			DESTINATION share/games/openspades/Resources)

	file(GLOB_RECURSE RESOURCES Shaders/* Scripts/* Icons/* Killfeed/*)
	foreach(FILE ${RESOURCES})
		string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${RESDIR} TARGETFILE ${FILE})
		get_filename_component(TARGETDIR ${TARGETFILE} PATH)
		add_custom_command(TARGET copy_resources COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGETDIR}")
		add_custom_command(TARGET copy_resources COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${TARGETFILE}")
	endforeach()
endif()

add_custom_target(OpenSpades_Resources ALL COMMENT "Preparing resources")

add_custom_command(
	TARGET OpenSpades_Resources
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/downloadpak.sh ${CMAKE_CURRENT_SOURCE_DIR}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Icons)

	add_custom_command(
		TARGET OpenSpades_Resources
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Icons/converticons.sh ${CMAKE_CURRENT_SOURCE_DIR}/Icons
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Icons)

endif(UNIX)
