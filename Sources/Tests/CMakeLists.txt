
set(SDL_MISCFILES ${OS_SRC_DIR}/Gui/SDLGLDevice.cpp ${OS_SRC_DIR}/Gui/SDLGLDevice.cpp)
set(CORE_MISCFILES ${OS_SRC_DIR}/Core/RefCountedObject.cpp ${OS_SRC_DIR}/Core/RefCountedObject.h ${OS_SRC_DIR}/Core/Math.cpp ${OS_SRC_DIR}/Core/Math.h
	${OS_SRC_DIR}/Core/Debug.cpp ${OS_SRC_DIR}/Core/Debug.h ${OS_SRC_DIR}/Core/Exception.cpp ${OS_SRC_DIR}/Core/Exception.h
	${OS_SRC_DIR}/Core/Settings.cpp ${OS_SRC_DIR}/Core/Settings.h ${OS_SRC_DIR}/Core/ConcurrentDispatch.cpp ${OS_SRC_DIR}/Core/ConcurrentDispatch.h
	${OS_SRC_DIR}/Core/Bitmap.cpp ${OS_SRC_DIR}/Core/Bitmap.h ${OS_SRC_DIR}/Core/VoxelModel.cpp ${OS_SRC_DIR}/Core/VoxelModel.h
	${OS_SRC_DIR}/Core/FileManager.cpp ${OS_SRC_DIR}/Core/FileManager.h ${OS_SRC_DIR}/Core/BitmapAtlasGenerator.cpp ${OS_SRC_DIR}/Core/BitmapAtlasGenerator.h
	${OS_SRC_DIR}/Core/IStream.cpp ${OS_SRC_DIR}/Core/IStream.h ${OS_SRC_DIR}/Core/Stopwatch.cpp ${OS_SRC_DIR}/Core/Stopwatch.h
	${OS_SRC_DIR}/Core/Thread.cpp ${OS_SRC_DIR}/Core/Thread.h ${OS_SRC_DIR}/Core/IBitmapCodec.cpp ${OS_SRC_DIR}/Core/IBitmapCodec.h
	${OS_SRC_DIR}/Core/Mutex.cpp ${OS_SRC_DIR}/Core/Mutex.h ${OS_SRC_DIR}/Core/ThreadLocalStorage.cpp ${OS_SRC_DIR}/Core/ThreadLocalStorage.h
	${OS_SRC_DIR}/Core/FileManager.cpp ${OS_SRC_DIR}/Core/FileManager.h ${OS_SRC_DIR}/Core/DirectoryFileSystem.cpp ${OS_SRC_DIR}/Core/DirectoryFileSystem.h
	${OS_SRC_DIR}/Core/ZipFileSystem.cpp ${OS_SRC_DIR}/Core/ZipFileSystem.h ${OS_SRC_DIR}/Core/StdStream.cpp ${OS_SRC_DIR}/Core/StdStream.h
	${OS_SRC_DIR}/Core/DynamicMemoryStream.cpp ${OS_SRC_DIR}/Core/DynamicMemoryStream.h ${OS_SRC_DIR}/Core/FltkImageReader.cpp ${OS_SRC_DIR}/Core/TargaReader.cpp)
	
set(CLIENT_MISCFILES ${OS_SRC_DIR}/Client/GameMap.cpp ${OS_SRC_DIR}/Client/GameMap.h ${OS_SRC_DIR}/Client/Quake3Font.cpp ${OS_SRC_DIR}/Client/Quake3Font.h
	${OS_SRC_DIR}/Client/IFont.cpp ${OS_SRC_DIR}/Client/IFont.h ${OS_SRC_DIR}/Client/ChatWindow.cpp ${OS_SRC_DIR}/Client/ChatWindow.h)

add_executable(DrawTest Main.cpp Draw.cpp ${SDL_MISCFILES} ${DRAW_FILES} ${CLIENT_MISCFILES} ${CORE_MISCFILES} ${POLY2TRI_FILES} ${POLY2TRI_COMMON_FILES} ${POLY2TRI_SWEEP_FILES}
				${KISS_FILES})

if(WIN32)
	foreach(LIB ${SDL_LIBRARY})
		string(REGEX REPLACE "\\.lib$" ".dll" SDL_DLL ${LIB})
		if(EXISTS "${SDL_DLL}")
			add_custom_command(TARGET OpenSpades POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_DLL}" "${CMAKE_BINARY_DIR}/bin/\$\(Configuration\)/")
			install(PROGRAMS ${SDL_DLL} DESTINATION bin)
		endif()
	endforeach()
	get_filename_component(GLEWLIBFILE ${GLEW_LIBRARY} NAME_WE)
	if("glew32s" STREQUAL ${GLEWLIBFILE})
		add_definitions(-DGLEW_STATIC)	#static linked
	endif()
	#lm: this sucks, msvc fucks up some linking (conflicts)
	set_target_properties(OpenSpades PROPERTIES LINK_FLAGS_RELEASE "/NODEFAULTLIB:\"libcmt.lib\" /NODEFAULTLIB:\"libcmtd.lib\"")
endif()




target_link_libraries(DrawTest ${SDL_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARY} ${FLTK_OS_LIBS} ${ZLIB_LIBRARIES})

set_target_properties(DrawTest PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(DrawTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(DrawTest PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(DrawTest PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

source_group("Draw" FILES ${DRAW_FILES})
source_group("Gui" FILES ${SDL_MISCFILES})
source_group("Core" FILES ${CORE_MISCFILES})
source_group("Client" FILES ${CLIENT_MISCFILES})
source_group("libs\\poly2tri" FILES ${POLY2TRI_FILES})
source_group("libs\\poly2tri\\common" FILES ${POLY2TRI_COMMON_FILES})
source_group("libs\\poly2tri\\sweep" FILES ${POLY2TRI_SWEEP_FILES})
source_group("libs\\kiss_fft130" FILES ${KISS_FILES})



