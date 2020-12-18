set(COMPILED_PAK_FILES
	${CMAKE_CURRENT_BINARY_DIR}/pak002-Base.pak
	${CMAKE_CURRENT_BINARY_DIR}/pak005-Models.pak
	${CMAKE_CURRENT_BINARY_DIR}/pak010-BaseSkin.pak
	${CMAKE_CURRENT_BINARY_DIR}/pak050-Locales.pak
	${CMAKE_CURRENT_BINARY_DIR}/pak999-References.pak)

set(PAK_FILES
	${COMPILED_PAK_FILES}
	${CMAKE_CURRENT_BINARY_DIR}/pak000-Nonfree.pak
	${CMAKE_CURRENT_BINARY_DIR}/font-unifont.pak
	PARENT_SCOPE)

if(OPENSPADES_RESOURCES)

	if (WIN32)
		# No automatic downloading for Windows (for now)
	elseif (UNIX)
		add_custom_target(OpenSpades_Resources_DevPaks ALL COMMENT "Downloading non-GPL assets"
			DEPENDS
				${CMAKE_CURRENT_BINARY_DIR}/pak000-Nonfree.pak
				${CMAKE_CURRENT_BINARY_DIR}/font-unifont.pak)
		add_custom_command(
			COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/downloadpak.sh
			OUTPUT
				${CMAKE_CURRENT_BINARY_DIR}/pak000-Nonfree.pak
				${CMAKE_CURRENT_BINARY_DIR}/font-unifont.pak
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# TODO: subgroups for script files
	file(GLOB_RECURSE SCRIPT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/*.as)
	file(GLOB_RECURSE SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.fs
		${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.program
		${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vs)
	set(PACKAGE_INFO_IN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/PackageInfo.json)
	set(PACKAGE_INFO_OUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/PackageInfo.json)

	source_group("Scripts" FILES ${SCRIPT_FILES})
	source_group("Shaders" FILES ${SHADER_FILES})

	add_custom_target(OpenSpades_Resources ALL
		COMMENT "Building assets"
		SOURCES ${SCRIPT_FILES} ${SHADER_FILES} ${PACKAGE_INFO_IN_FILE}
		DEPENDS 
			${PACKAGE_INFO_OUT_FILE}
			${COMPILED_PAK_FILES})

	if(WIN32)
		if(MSVC)
			# multi-config
			# FIXME: The actual output directory is
			#   `${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Resources`, but
			#   the `OUTPUT` parameter (`${COMPILED_PAK_FILES}`) points to
			#   `${CMAKE_BINARY_DIR}/Resources`. Consequently, MSVC runs this
			#   command on every build run whether the input files have changed
			#   or not.
			#   CMake does not support generator expressions (`$<...>`) in
			#   the `OUTPUT` parameter of `add_custom_command`.
			add_custom_command(
				OUTPUT ${COMPILED_PAK_FILES}
				COMMAND powershell -ExecutionPolicy Bypass ${CMAKE_CURRENT_SOURCE_DIR}/mkpak.ps1
					${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Resources
					DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		else()
			add_custom_command(
				OUTPUT ${COMPILED_PAK_FILES}
				COMMAND powershell -ExecutionPolicy Bypass ${CMAKE_CURRENT_SOURCE_DIR}/mkpak.ps1
					${CMAKE_BINARY_DIR}/bin/Resources
					DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		endif()
	elseif (UNIX)
		add_custom_command(
			OUTPUT ${COMPILED_PAK_FILES}
			COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mkpak.sh
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	if (WIN32 AND MSVC)
		# multi-config
		# FIXME: see above
		add_custom_command(
			OUTPUT ${PACKAGE_INFO_OUT_FILE}
			DEPENDS ${PACKAGE_INFO_IN_FILE}
			COMMAND ${CMAKE_COMMAND} -E copy
				${PACKAGE_INFO_IN_FILE} 
				${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Resources/PackageInfo.json)
	else (WIN32 AND MSVC)
		add_custom_command(
			OUTPUT ${PACKAGE_INFO_OUT_FILE}
			DEPENDS ${PACKAGE_INFO_IN_FILE}
			COMMAND ${CMAKE_COMMAND} -E copy
				${PACKAGE_INFO_IN_FILE} ${PACKAGE_INFO_OUT_FILE})
	endif (WIN32 AND MSVC)
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
