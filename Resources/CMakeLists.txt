

if(RESDIR)
	add_custom_target(copy_resources ALL COMMENT "Copying resources")

	file(GLOB_RECURSE SHADERS Shaders/*)
	foreach(FILE ${SHADERS})
		string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${RESDIR} TARGETFILE ${FILE})
		get_filename_component(TARGETDIR ${TARGETFILE} PATH)
		add_custom_command(TARGET copy_resources COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGETDIR}")
		add_custom_command(TARGET copy_resources COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${TARGETFILE}")
	endforeach()

endif()
