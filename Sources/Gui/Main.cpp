/*
 Copyright (c) 2013 yvt
 
 This file is part of OpenSpades.
 
 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 
 */

#include <OpenSpades.h>
#include "MainWindow.h"
#include <Core/FileManager.h>
#include <Core/DirectoryFileSystem.h>
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/ZipFileSystem.h>
#include "ErrorDialog.h"

#include <Core/VoxelModel.h>
#include <Draw/GLOptimizedVoxelModel.h>

#include <Core/ScriptManager.h>

#include <algorithm>	//std::sort

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#endif

#ifdef __APPLE__
#include <xmmintrin.h>
#endif

#include <FL/fl_ask.H>

int main(int argc, char ** argv)
{
	
	// Enable FPE
#if 0
#ifdef __APPLE__
	short fpflags = 0x1332; // Default FP flags, change this however you want.
	__asm__("fnclex\n\tfldcw %0\n\t": "=m"(fpflags));
	
	_MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);
#endif
	
#endif
	try{
		
		Fl::scheme("gtk+");
		
		spades::reflection::Backtrace::StartBacktrace();
		
		SPADES_MARK_FUNCTION();
		spades::DispatchQueue::GetThreadQueue()->MarkSDLVideoThread();
		
		SPLog("Package: " PACKAGE_STRING);
		
		// default resource directories
#ifdef WIN32
		static char buf[4096];
		GetModuleFileName(NULL, buf, 4096);
		std::string appdir = buf;
		appdir = appdir.substr(0, appdir.find_last_of('\\')+1);
		
		spades::FileManager::AddFileSystem
		(new spades::DirectoryFileSystem(appdir + "Resources", false));
		
		if(SUCCEEDED(SHGetFolderPath(NULL,
									 CSIDL_APPDATA,
									 NULL, 0,
									 buf))){
			
			
			std::string datadir = buf;
			datadir += "\\OpenSpades\\Resources";
			spades::FileManager::AddFileSystem
			(new spades::DirectoryFileSystem(datadir, true));
		}
#elif defined(__APPLE__)
		std::string home = getenv("HOME");
		spades::FileManager::AddFileSystem
		(new spades::DirectoryFileSystem("./Resources", false));
		
		spades::FileManager::AddFileSystem
		(new spades::DirectoryFileSystem(home+"/Library/Application Support/OpenSpades/Resources", true));
#else
		std::string home = getenv("HOME");
		spades::FileManager::AddFileSystem
		(new spades::DirectoryFileSystem("./Resources", false));
		
		spades::FileManager::AddFileSystem
		(new spades::DirectoryFileSystem(home+"/.openspades/Resources", true));
#endif
		
		try{
			spades::StartLog();
		}catch(const std::exception& ex){
			fl_alert("Failed to start recording log because of the following error:\n%s\n\n"
					 "OpenSpades will continue to run, but any critical events are not logged.", ex.what());
		}
		SPLog("Log Started.");
		
#if defined(RESDIR_DEFINED)
		spades::FileManager::AddFileSystem
		(new spades::DirectoryFileSystem(RESDIR, false));
#endif
		
		/*
		 spades::FileManager::AddFileSystem
		 (new spades::DirectoryFileSystem("/Users/tcpp/Programs/MacPrograms2/OpenSpades/Resources", false));
		 */
		// add all zip files
		{
			std::vector<spades::IFileSystem*> fss;
			
			std::vector<std::string> files = spades::FileManager::EnumFiles("");
			
			struct Comparator {
				static int GetPakId(const std::string& str) {
					if(str.size() >= 4 && str[0] == 'p' &&
					   str[1] == 'a' && str[2] == 'k' &&
					   (str[3] >= '0' && str[3] <= '9')){
						return atoi(str.c_str() + 3);
					}else{
						return 32767;
					}
				}
				static bool Compare(const std::string& a,
									const std::string& b) {
					int pa = GetPakId(a);
					int pb = GetPakId(b);
					if(pa == pb){
						return a < b;
					}else{
						return pa < pb;
					}
				}
			};
			
			std::sort(files.begin(), files.end(), Comparator::Compare);
			
			for(size_t i = 0; i < files.size(); i++){
				std::string name = files[i];
				
				// check extension
				if(name.size() < 4 ||
				   name.rfind(".pak") != name.size() - 4){
					continue;
				}
				
				if(spades::FileManager::FileExists(name.c_str())) {
					spades::IStream *stream = spades::FileManager::OpenForReading(name.c_str());
					spades::ZipFileSystem *fs = new spades::ZipFileSystem(stream);
					SPLog("Pak Registered: %s\n", name.c_str());
					fss.push_back(fs);
				}
			}
			for(size_t i = 0; i < fss.size(); i++){
				spades::FileManager::AddFileSystem(fss[i]);
			}
		}
		
		SPLog("Initializing script engine");
		spades::ScriptManager::GetInstance();
		
		SPLog("Initializing main window");
		
		MainWindow win;
		win.Init();
		win.show(argc, argv);
		win.CheckGLCapability();
		
		SPLog("Entering FLTK main loop");
		Fl::run();
		
		SPLog("Leaving FLTK main loop");
		spades::Settings::GetInstance()->Flush();
		
	}catch(const std::exception& ex) {
		
		ErrorDialog dlg;
		dlg.set_modal();
		dlg.result = 0;
		
		// TODO: free this buffer (just leaking)
		Fl_Text_Buffer *buf = new Fl_Text_Buffer;
		buf->append(ex.what());
		dlg.infoView->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
		dlg.infoView->buffer(buf);
		dlg.helpView->value("See SystemMessages.log for more details.");
		dlg.show();
		while(dlg.visible()){
			Fl::wait();
		}
		
	}
	
    return 0;
}

