
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


if(OPENSPADES_RESOURCES)
	add_custom_target(OpenSpades_Resources ALL COMMENT "Downloading external resources")

	add_custom_command(
		TARGET OpenSpades_Resources
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/downloadpak.sh ${CMAKE_CURRENT_SOURCE_DIR}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

endif()

if(UNIX)
	add_custom_target(OpenSpades_Icons ALL COMMENT "Generating icons")
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Icons)
	add_custom_command(
		TARGET OpenSpades_Icons
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Icons/converticons.sh ${CMAKE_CURRENT_SOURCE_DIR}/Icons
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Icons)

	set(ICON_DIR ${CMAKE_CURRENT_BINARY_DIR}/Icons/hicolor/16x16/apps)
	add_library(OpenSpades_BinaryExecutableIcon
		STATIC
		${ICON_DIR}/icon.o
	)
	set_target_properties(
		OpenSpades_BinaryExecutableIcon
		PROPERTIES
		LINKER_LANGUAGE CXX
	)
	set_source_files_properties(
		${ICON_DIR}/icon.o
		PROPERTIES
		EXTERNAL_OBJECT true
		GENERATED true
	)
	add_custom_command(
		OUTPUT ${ICON_DIR}/icon.o
		COMMAND cd ${ICON_DIR} && ld -r -b binary -o icon.o openspades.png
		COMMAND objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents ${ICON_DIR}/icon.o ${ICON_DIR}/icon.o
		DEPENDS OpenSpades_Icons
	)
endif(UNIX)
