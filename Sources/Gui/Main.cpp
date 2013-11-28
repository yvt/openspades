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
#include <Imports/SDL.h>
#include "Main.h"
#include "MainWindow.h"
#include "MainScreen.h"
#include <Core/FileManager.h>
#include <Core/DirectoryFileSystem.h>
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Thread.h>
#include <Core/ZipFileSystem.h>
#include <Core/ServerAddress.h>
#include "ErrorDialog.h"
#include "Runner.h"
#include <Client/GameMap.h>
#include <Client/Client.h>

#include <Core/VoxelModel.h>
#include <Draw/GLOptimizedVoxelModel.h>

#include <ScriptBindings/ScriptManager.h>

#include <algorithm>	//std::sort

SPADES_SETTING(ui_forceClassicMainWindow, "0");

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#define strncasecmp(x,y,z)	_strnicmp(x,y,z)
#define strcasecmp(x,y)		_stricmp(x,y)

//lm: without doing it this way, we will get a low-res icon or an ugly resampled icon in our window.
//we cannot use the fltk function on the console window, because it's not an Fl_Window...
void setIcon( HWND hWnd )
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HICON hIcon = (HICON)LoadImageA( hInstance, "AppIcon", IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0 );
	if( hIcon ) {
		SendMessage( hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon );
	}
	hIcon = (HICON)LoadImageA( hInstance, "AppIcon", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0 );
	if( hIcon ) {
		SendMessage( hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon );
	}
}

#endif

//fltk
void setWindowIcon( Fl_Window* window )
{
#ifdef _WIN32
	window->icon( (char *)LoadIconA( GetModuleHandle(NULL), "AppIcon" ) );
#else
	//check for mac / linux icon with fltk?
#endif
}

#ifdef __APPLE__
#include <xmmintrin.h>
#endif

#include <FL/fl_ask.H>

SPADES_SETTING(cg_lastQuickConnectHost, "");
SPADES_SETTING(cg_protocolVersion, "");
int cg_autoConnect = 0;

int argsHandler(int argc, char **argv, int &i)
{
	if( char* a = argv[i] ) {
		if( !strncasecmp( a, "aos://", 6 ) ) {
			cg_lastQuickConnectHost = a;
			cg_autoConnect = 1;
			return ++i;
		}
		//lm: we attempt to detect protocol version, allowing with or without a prefix 'v='
		//we set proto, but without url we will not auto-connect
		if( a[0] == 'v' && a[1] == '=' ) { a += 2; }
		if( !strcasecmp( a, "75" ) || !strcasecmp( a, "075" ) || !strcasecmp( a, "0.75" ) ) {
			cg_protocolVersion = 3;
			return ++i;
		}
		if( !strcasecmp( a, "76" ) || !strcasecmp( a, "076" ) || !strcasecmp( a, "0.76" ) ) {
			cg_protocolVersion = 4;
			return ++i;
		}
	}
	return 0;
}

namespace spades {
	void StartClient(const spades::ServerAddress& addr, const std::string& playerName){
		class ConcreteRunner: public spades::gui::Runner {
			spades::ServerAddress addr;
			std::string playerName;
		protected:
			virtual spades::gui::View *CreateView(spades::client::IRenderer *renderer, spades::client::IAudioDevice *audio) {
				return new spades::client::Client(renderer, audio, addr, playerName);
			}
		public:
			ConcreteRunner(const spades::ServerAddress& addr,
								  const std::string& playerName):
			addr(addr), playerName(playerName){ }
		};
		ConcreteRunner runner(addr, playerName);
		runner.RunProtected();
	}
	void StartMainScreen(){
		class ConcreteRunner: public spades::gui::Runner {
		protected:
			virtual spades::gui::View *CreateView(spades::client::IRenderer *renderer, spades::client::IAudioDevice *audio) {
				return new spades::gui::MainScreen(renderer, audio);
			}
		public:
		};
		ConcreteRunner runner;
		runner.RunProtected();
	}
}

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
		spades::Thread::InitThreadSystem();
		spades::DispatchQueue::GetThreadQueue()->MarkSDLVideoThread();
		
		SPLog("Package: " PACKAGE_STRING);
		
		// default resource directories
#ifdef WIN32
		static char buf[4096];
		GetModuleFileName(NULL, buf, 4096);
		std::string appdir = buf;
		appdir = appdir.substr(0, appdir.find_last_of('\\')+1);
		
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(appdir + "Resources", false));
		
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, buf))){
			std::string datadir = buf;
			datadir += "\\OpenSpades\\Resources";
			spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(datadir, true));
		}
		//fltk has a console window on windows (can disable while building, maybe use a builtin console for a later release?)
		HWND hCon = GetConsoleWindow();
		if( NULL != hCon ) {
			setIcon( hCon );
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
		
#if defined(RESDIR_DEFINED) && !NDEBUG
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(RESDIR, false));
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
			for(size_t i = fss.size(); i > 0; i--){
				spades::FileManager::AddFileSystem(fss[i - 1]);
			}
		}
		
		SPLog("Initializing script engine");
		spades::ScriptManager::GetInstance();
		
		SPLog("Initializing window system");
		int dum = 0;
		Fl::args( argc, argv, dum, argsHandler );
		
		// MAPGEN
		/*spades::client::GameMap *m = spades::client::GameMap::Load(spades::FileManager::OpenForReading("Maps/shot0000.vxl"));
		for(int x = 0; x < 512; x++) {
			for(int y = 0; y < 512; y++) {
				for(int z = 0; z < 64; z++) {
					if(m->IsSolid(x, y, z)) {
						uint32_t col = m->GetColor(x, y, z);
						int bri = (col & 0xff) + ((col >> 8) & 0xff) + ((col >> 16) & 0xff);
						bri /= 3;
						col = 0xff000000 + bri * 0x010101;
						m->Set(x, y, z, true, col);
					}
				}
			}
		}
		auto outs = spades::FileManager::OpenForWriting("Maps/Title.vxl");
		m->Save(outs);
		delete outs;
		return 0;*/

		MainWindow* win = NULL;
		if( !cg_autoConnect ) {
			if(!((int)ui_forceClassicMainWindow ||
				 Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))) {
				// TODO: always show main window for first run
				
				SPLog("Starting main screen");
				spades::StartMainScreen();
			}else{
				SPLog("Initializing main window");
				win = new MainWindow();
				win->Init();
				setWindowIcon( win );
				win->show(argc, argv);
				win->CheckGLCapability();
				
				SPLog("Entering FLTK main loop");
				Fl::run();
				SPLog("Leaving FLTK main loop");
			}
		} else {
			spades::ServerAddress host(cg_lastQuickConnectHost.CString(), (int)cg_protocolVersion == 3 ? spades::ProtocolVersion::v075 : spades::ProtocolVersion::v076 );
			MainWindow::StartGame( host );
		}
		
		spades::Settings::GetInstance()->Flush();
		
		if( win ) {
			delete win;
		}

	}catch(const std::exception& ex) {
		
		ErrorDialog dlg;
		setWindowIcon( &dlg );
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

