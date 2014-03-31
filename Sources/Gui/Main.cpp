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
#include "MainScreen.h"
#include <Core/FileManager.h>
#include <Core/DirectoryFileSystem.h>
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Thread.h>
#include <Core/ZipFileSystem.h>
#include <Core/ServerAddress.h>
#include "Runner.h"
#include <Client/GameMap.h>
#include <Client/Client.h>
#include <Core/CpuID.h>
#include <Gui/StartupScreen.h>
#include <Core/Strings.h>

#include <Core/VoxelModel.h>
#include <Draw/GLOptimizedVoxelModel.h>

#include <ScriptBindings/ScriptManager.h>

#include <algorithm>	//std::sort
#include <FL/Fl.H>

SPADES_SETTING(cl_showStartupWindow, "");

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#define strncasecmp(x,y,z)	_strnicmp(x,y,z)
#define strcasecmp(x,y)		_stricmp(x,y)

SPADES_SETTING(core_win32BeginPeriod, "1");

class ThreadQuantumSetter {
public:
	ThreadQuantumSetter() {
		if(core_win32BeginPeriod){
			timeBeginPeriod(1);
			SPLog("Thread quantum was modified to 1ms by timeBeginPeriod");
			SPLog("(to disable this behavior, set core_win32BeginPeriod to 0)");
		}else{
			SPLog("Thread quantum is not modified");
			SPLog("(to enable this behavior, set core_win32BeginPeriod to 1)");
		}
	}
	~ThreadQuantumSetter() {
		if(core_win32BeginPeriod){
			timeEndPeriod(1);
			SPLog("Thread quantum was restored");
		}
	}
};

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

#include <DbgHelp.h>

LONG WINAPI UnhandledExceptionProc( LPEXCEPTION_POINTERS lpEx )
{
	typedef BOOL (WINAPI* PDUMPFN)( HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam );
	HMODULE hLib = LoadLibrary( "DbgHelp.dll" );
	PDUMPFN pMiniDumpWriteDump = (PDUMPFN)GetProcAddress(hLib, "MiniDumpWriteDump");

	static char buf[MAX_PATH+120] = {0};	//this is our display buffer.
	if( pMiniDumpWriteDump ) {
		static char fullBuf[MAX_PATH+120] = {0};
		if( SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, buf)) ){	//max length = MAX_PATH (temp abuse this buffer space)
			strcat_s( buf, "\\" );	// ensure we end with a slash.
		} else {
			buf[0] = 0;	//empty it, the file will now end up in the working directory :(
		}
		sprintf( fullBuf, "%sOpenSpadesCrash%d.dmp", buf, GetTickCount() );		//some sort of randomization.
		HANDLE hFile = CreateFile( fullBuf, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ); 
		if( hFile != INVALID_HANDLE_VALUE ) {
			MINIDUMP_EXCEPTION_INFORMATION mdei = {0};
			mdei.ThreadId = GetCurrentThreadId();
			mdei.ExceptionPointers = lpEx;
			mdei.ClientPointers = TRUE;
			MINIDUMP_TYPE mdt = MiniDumpNormal;
			BOOL rv = pMiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (lpEx != 0) ? &mdei : 0, 0, 0 );
			CloseHandle( hFile );
			sprintf_s( buf, "Something went horribly wrong, please send the file \n%s\nfor analysis.", fullBuf );
		} else {
			sprintf_s( buf, "Something went horribly wrong,\ni even failed to store information about the problem... (0x%08x)", lpEx ? lpEx->ExceptionRecord->ExceptionCode : 0xffffffff );
		}
	} else {
		sprintf_s( buf, "Something went horribly wrong,\ni even failed to retrieve information about the problem... (0x%08x)", lpEx ? lpEx->ExceptionRecord->ExceptionCode : 0xffffffff );
	}
	MessageBoxA( NULL, buf, "Oops, we crashed...", MB_OK | MB_ICONERROR );
	ExitProcess( -1 );
	//return EXCEPTION_EXECUTE_HANDLER;
}
#else

class ThreadQuantumSetter {
	
};

#endif

#ifdef __APPLE__
#include <xmmintrin.h>
#endif

SPADES_SETTING(cg_lastQuickConnectHost, "");
SPADES_SETTING(cg_protocolVersion, "");
SPADES_SETTING(cg_playerName, "");
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
#ifdef WIN32
	SetUnhandledExceptionFilter( UnhandledExceptionProc );
#endif
	// Enable FPE
#if 0
#ifdef __APPLE__
	short fpflags = 0x1332; // Default FP flags, change this however you want.
	__asm__("fnclex\n\tfldcw %0\n\t": "=m"(fpflags));
	
	_MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);
#endif
	
#endif
	try{
		
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
		
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, buf))){
			std::string datadir = buf;
			datadir += "\\OpenSpades\\Resources";
			spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(datadir, true));
		}
		
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(appdir + "Resources", false));
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
			
			SDL_InitSubSystem(SDL_INIT_VIDEO);
			auto msg = spades::Format("Failed to start recording log because of the following error:\n{0}\n\n"
									  "OpenSpades will continue to run, but any critical events are not logged.", ex.what());
			if(SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING,
										"OpenSpades Log System Failure",
										msg.c_str(), nullptr)) {
				// showing dialog failed.
				// TODO: do appropriate action?
			}
		}
		SPLog("Log Started.");
		
		spades::Settings::GetInstance()->Load();
		
		{
			spades::CpuID cpuid;
			SPLog("---- CPU Information ----");
			SPLog("Vendor ID: %s", cpuid.GetVendorId().c_str());
			SPLog("Brand ID: %s", cpuid.GetBrand().c_str());
			SPLog("Supports MMX: %s", cpuid.Supports(spades::CpuFeature::MMX)?"YES":"NO");
			SPLog("Supports SSE: %s", cpuid.Supports(spades::CpuFeature::SSE)?"YES":"NO");
			SPLog("Supports SSE2: %s", cpuid.Supports(spades::CpuFeature::SSE2)?"YES":"NO");
			SPLog("Supports SSE3: %s", cpuid.Supports(spades::CpuFeature::SSE3)?"YES":"NO");
			SPLog("Supports SSSE3: %s", cpuid.Supports(spades::CpuFeature::SSSE3)?"YES":"NO");
			SPLog("Supports FMA: %s", cpuid.Supports(spades::CpuFeature::FMA)?"YES":"NO");
			SPLog("Supports AVX: %s", cpuid.Supports(spades::CpuFeature::AVX)?"YES":"NO");
			SPLog("Supports AVX2: %s", cpuid.Supports(spades::CpuFeature::AVX2)?"YES":"NO");
			SPLog("Supports AVX512F: %s", cpuid.Supports(spades::CpuFeature::AVX512F)?"YES":"NO");
			SPLog("Supports AVX512CD: %s", cpuid.Supports(spades::CpuFeature::AVX512CD)?"YES":"NO");
			SPLog("Supports AVX512ER: %s", cpuid.Supports(spades::CpuFeature::AVX512ER)?"YES":"NO");
			SPLog("Supports AVX512PF: %s", cpuid.Supports(spades::CpuFeature::AVX512PF)?"YES":"NO");
			SPLog("Simultaneous Multithreading: %s", cpuid.Supports(spades::CpuFeature::SimultaneousMT)?"YES":"NO");
			SPLog("Misc:");
			SPLog("%s", cpuid.GetMiscInfo().c_str());
			SPLog("-------------------------");
		}
		
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
			std::vector<spades::IFileSystem*> fssImportant;
			
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
					if(name[0] == '_' && false) { // last resort for #198
						SPLog("Pak Registered: %s (marked as 'important')\n", name.c_str());
						fssImportant.push_back(fs);
					}else{
						SPLog("Pak Registered: %s\n", name.c_str());
						fss.push_back(fs);
					}
				}
			}
			for(size_t i = fss.size(); i > 0; i--){
				spades::FileManager::AppendFileSystem(fss[i - 1]);
			}
			for(size_t i = 0; i < fssImportant.size(); i++){
				spades::FileManager::PrependFileSystem(fssImportant[i]);
			}
		}
		
		SPLog("Initializing localization system");
		spades::LoadCurrentLocale();
		_Tr("Main", "Localization System Loaded");
		
		SPLog("Initializing script engine");
		spades::ScriptManager::GetInstance();
		
		SPLog("Initializing window system");
		int dum = 0;
		Fl::args( argc, argv, dum, argsHandler );

		ThreadQuantumSetter quantumSetter;
		(void)quantumSetter; // suppress "unused variable" warning
		
		if( !cg_autoConnect ) {
			if(!((int)cl_showStartupWindow != 0 ||
				 Fl::get_key(FL_Shift_L) || Fl::get_key(FL_Shift_R))) {
				// TODO: always show main window for first run
				
				SPLog("Starting main screen");
				spades::StartMainScreen();
			}else{
				SPLog("Starting startup window");
				::spades::gui::StartupScreen::Run();
			}
		} else {
			spades::ServerAddress host(cg_lastQuickConnectHost.CString(), (int)cg_protocolVersion == 3 ? spades::ProtocolVersion::v075 : spades::ProtocolVersion::v076 );
			spades::StartClient(host, cg_playerName);
		}
		
		spades::Settings::GetInstance()->Flush();

	}catch(const std::exception& ex) {
		
		std::string msg = ex.what();
		msg = _Tr("Main", "A serious error caused OpenSpades to stop working:\n\n{0}\n\nSee SystemMessages.log for more details.", msg);
		
		SPLog("[!] Terminating due to the fatal error: %s", ex.what());
		
		SDL_InitSubSystem(SDL_INIT_VIDEO);
		if(SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, _Tr("Main", "OpenSpades Fatal Error").c_str(), msg.c_str(), nullptr)) {
			// showing dialog failed.
			// TODO: do appropriate action
		}
		
	}
	
    return 0;
}

