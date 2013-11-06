#include <OpenSpades.h>
#include <Gui/SDLGLDevice.h>
#include <Core/RefCountedObject.h>
#include <Draw/GLRenderer.h>
#include <Core/Settings.h>
#include <FL/Fl_Preferences.H>
#include <Core/FileManager.h>
#include <Core/DirectoryFileSystem.h>
#include <Core/ZipFileSystem.h>
#include <Core/ConcurrentDispatch.h>

#include <iostream>
#include <algorithm>

#ifdef WIN32
#include <Windows.h>
#include <shlobj.h>
#endif

SPADES_SETTING(r_depthBits, "16");
SPADES_SETTING(r_colorBits, "32");
SPADES_SETTING(r_videoWidth, "1024");
SPADES_SETTING(r_videoHeight, "640");
SPADES_SETTING(r_fullscreen, "0");

//lm: meh, don't care about this stuff in a test project, so just go with dummy implementation
extern "C" int asAtomicInc(int &value) { return ++value; }

void drawFrame( spades::client::IRenderer& renderer );


#undef main		//fukn SDL re-defining shit.

int main()
{
	try{
		spades::reflection::Backtrace::StartBacktrace();
		SPADES_MARK_FUNCTION();
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
#elif defined(__APPLE__)
		std::string home = getenv("HOME");
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem("./Resources", false));
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(home+"/Library/Application Support/OpenSpades/Resources", true));
#else
		std::string home = getenv("HOME");
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem("./Resources", false));
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(home+"/.openspades/Resources", true));
#endif
		
		try{
			spades::StartLog();
		}catch(const std::exception& ex){
			SPLog( "Exception: %s", ex.what() );
		}
		SPLog("Log Started.");
		
#if defined(RESDIR_DEFINED) && !NDEBUG
		spades::FileManager::AddFileSystem(new spades::DirectoryFileSystem(RESDIR, false));
#endif

		{
			std::vector<spades::IFileSystem*> fss;
			
			std::vector<std::string> files = spades::FileManager::EnumFiles("");
			
			struct Comparator {
				static int GetPakId(const std::string& str) {
					if(str.size() >= 4 && str[0] == 'p' && str[1] == 'a' && str[2] == 'k' && (str[3] >= '0' && str[3] <= '9')){
						return atoi(str.c_str() + 3);
					}else{
						return 32767;
					}
				}
				static bool Compare(const std::string& a, const std::string& b) {
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
				if(name.size() < 4 || name.rfind(".pak") != name.size() - 4){
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


		SDL_Init(SDL_INIT_VIDEO);
		SDL_WM_SetCaption( "GFX Test app", "GFX Test app" );

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, (int)r_depthBits);
		int flags = SDL_OPENGL | SDL_DOUBLEBUF;
		if( (int)r_fullscreen ) {
			flags |= SDL_FULLSCREEN;
		}
		SDL_Surface *surface = SDL_SetVideoMode( (int)r_videoWidth, (int)r_videoHeight, (int)r_colorBits, flags );
		if(!surface){
			std::cerr << "Failed to initialize video device: " << SDL_GetError();
			return 1;
		}

		SDL_WM_GrabInput(SDL_GRAB_ON);
		SDL_EnableUNICODE(1);
		SDL_ShowCursor(0);
		{
			spades::gui::SDLGLDevice glDevice(surface);
			spades::Handle<spades::draw::GLRenderer> renderer = new spades::draw::GLRenderer(&glDevice);
			bool running = true;
				while(running){
					spades::DispatchQueue::GetThreadQueue()->ProcessQueue();

					drawFrame( *renderer );

					SDL_Event event;
					while(SDL_PollEvent(&event)) {
						switch (event.type) {
							case SDL_QUIT:
								running = false;
								break;
							case SDL_KEYUP:
								if( SDLK_ESCAPE == event.key.keysym.sym ) {
									running = false;
								}
								break;
							case SDL_ACTIVEEVENT:
								if(event.active.gain){
									SDL_WM_GrabInput( SDL_GRAB_ON );
								}else{
									SDL_WM_GrabInput( SDL_GRAB_OFF );
								}
								break;
							default:
								break;
						}
					}
				}
			renderer->Shutdown();
		}
	}catch(...){
		SDL_Quit();
	}
	SDL_Quit();
	return 0;
}

