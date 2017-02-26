
if(OPENSPADES_RESOURCES)

	if (WIN32)
		# No automatic downloading for Windows (for now)
	elseif (UNIX)
		add_custom_target(OpenSpades_Resources_DevPaks ALL COMMENT "Downloading non-GPL assets")
		add_custom_command(
			TARGET OpenSpades_Resources_DevPaks
			COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/downloadpak.sh
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# TODO: subgroups for script files
	file(GLOB_RECURSE SCRIPT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/*.as)
	file(GLOB_RECURSE SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.fs
		${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.program
		${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vs)

	source_group("Scripts" FILES ${SCRIPT_FILES})
	source_group("Shaders" FILES ${SHADER_FILES})

	add_custom_target(OpenSpades_Resources ALL
		COMMENT "Packing assets"
		SOURCES ${SCRIPT_FILES} ${SHADER_FILES}
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR})

	if(WIN32)
		if(MSVC)
			# multi-config
			add_custom_command(
				TARGET OpenSpades_Resources
				COMMAND powershell -ExecutionPolicy Bypass ${CMAKE_CURRENT_SOURCE_DIR}/mkpak.ps1
					${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Resources
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		else()
			add_custom_command(
				TARGET OpenSpades_Resources
				COMMAND powershell -ExecutionPolicy Bypass ${CMAKE_CURRENT_SOURCE_DIR}/mkpak.ps1
					${CMAKE_BINARY_DIR}/bin/Resources
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		endif()
	elseif (UNIX)
		add_custom_command(
			TARGET OpenSpades_Resources
			COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mkpak.sh
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/PackageInfo.json
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(UNIX AND NOT APPLE)
	add_custom_target(OpenSpades_Unix ALL COMMENT "Process unix stuff")

	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Icons)
	add_custom_command(
		TARGET OpenSpades_Unix
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Icons/converticons.sh ${CMAKE_CURRENT_SOURCE_DIR}/Icons
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Icons)

	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Unix)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Unix/Debian)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Unix/Man)

	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Unix/Debian/copyright.in" "${CMAKE_CURRENT_BINARY_DIR}/Unix/Debian/copyright")
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Unix/Man/openspades.6.in" "${CMAKE_CURRENT_BINARY_DIR}/Unix/Man/openspades.6")

	add_custom_command(
		TARGET OpenSpades_Unix
		COMMAND sh -c \"gzip -9 -c ${CMAKE_CURRENT_SOURCE_DIR}/Unix/Debian/changelog > changelog.gz\"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Unix/Debian
	)
	add_custom_command(
		TARGET OpenSpades_Unix
		COMMAND sh -c \"gzip -9 -c ${CMAKE_CURRENT_BINARY_DIR}/Unix/Man/openspades.6 > openspades.6.gz\"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Unix/Man
	)
endif(UNIX AND NOT APPLE)
