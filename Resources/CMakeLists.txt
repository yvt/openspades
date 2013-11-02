

if(RESDIR_DEFINED)
	add_custom_target(copy_resources ALL COMMENT "Copying resources")
	install(FILES
		${RESDIR}/pak000-Base.pak
		${RESDIR}/pak001-Sounds.pak
		${RESDIR}/pak002-Models.pak
		${RESDIR}/pak010-BaseSkin.pak
		${RESDIR}pak999-References.pak
			DESTINATION bin/Resources)

	file(GLOB_RECURSE RESOURCES Shaders/* Scripts/*)
	foreach(FILE ${RESOURCES})
		string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${RESDIR} TARGETFILE ${FILE})
		get_filename_component(TARGETDIR ${TARGETFILE} PATH)
		add_custom_command(TARGET copy_resources COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGETDIR}")
		add_custom_command(TARGET copy_resources COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${TARGETFILE}")
	endforeach()
endif()

install(DIRECTORY "Scripts" DESTINATION bin/Resources)
install(DIRECTORY "Shaders" DESTINATION bin/Resources)
