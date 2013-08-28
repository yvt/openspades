//
//  MainWindowHandler.cpp
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "MainWindow.h"
#include <stdlib.h>

#include "../Core/Debug.h"
#include "SDLRunner.h"
#include <FL/fl_ask.H>
#include "../Core/Settings.h"
#include "../Imports/SDL.h"
#include <FL/Fl_PNG_Image.H>
#include "../Core/FileManager.h"
#include "../Core/IStream.h"
#include "SDLAsyncRunner.h"
#include "DetailConfigWindow.h"
#include "../Core/Math.h"

#include "../Imports/OpenGL.h" //for gpu info

using namespace spades::gui;

SPADES_SETTING(cg_smp, "0");
SPADES_SETTING(cg_blood, "1");
SPADES_SETTING(cg_lastQuickConnectHost, "127.0.0.1");
SPADES_SETTING(cg_playerName, "Deuce");
SPADES_SETTING(r_bloom, "1");
SPADES_SETTING(r_lens, "1");
SPADES_SETTING(r_cameraBlur, "1");
SPADES_SETTING(r_softParticles, "1");
SPADES_SETTING(r_mapSoftShadow, "0");
SPADES_SETTING(r_modelShadows, "1");
SPADES_SETTING(r_radiosity, "0");
SPADES_SETTING(r_dlights, "1");
SPADES_SETTING(r_water, "1");
SPADES_SETTING(r_multisamples, "0");
SPADES_SETTING(r_fxaa, "1");
SPADES_SETTING(r_depthBits, "24");
SPADES_SETTING(r_colorBits, "0");
SPADES_SETTING(r_videoWidth, "1024");
SPADES_SETTING(r_videoHeight, "640");
SPADES_SETTING(r_fullscreen, "0");
SPADES_SETTING(r_fogShadow, "0");
SPADES_SETTING(r_lensFlare, "1");
SPADES_SETTING(r_srgb, "1");
SPADES_SETTING(s_maxPolyphonics, "96");
SPADES_SETTING(s_eax, "1");

static std::vector<spades::IntVector3> g_modes;

void MainWindow::StartGame(const std::string &host) {
	SPADES_MARK_FUNCTION();
	
	hide();
	
#if 0
	SDLRunner r(host);
	r.Run();
#else
	
	try{
		if(cg_smp){
			SDLAsyncRunner r(host, cg_playerName);
			r.Run();
		}else{
			SDLRunner r(host, cg_playerName);
			r.Run();
		}
	}catch(const spades::Exception& ex){
		SPLog("Unhandled exception in SDLRunner:\n%s", ex.what());
		fl_message("Game was terminated due to an unexpected error:\n\n%s\n\nSee SystemMessages.log for more details.", ex.GetShortMessage().c_str());
	}catch(const std::exception& ex){
		SPLog("Unhandled exception in SDLRunner:\n%s", ex.what());
		fl_message("Game was terminated due to an unexpected error:\n\n%s\n\nSee SystemMessages.log for more details.", ex.what());
	}
#endif

}

void MainWindow::QuickConnectPressed() {
	SPADES_MARK_FUNCTION();
	
	StartGame(quickHostInput->value());
}

#pragma mark - Setup

void MainWindow::LoadPrefs() {
	SPADES_MARK_FUNCTION();
	
	SPLog("Loading Preferences to MainWindow");
	
	// --- video
	// add modes
	char buf[64];
	modeSelect->clear();
	for(size_t i = 0; i < g_modes.size(); i++){
		spades::IntVector3 mode = g_modes[i];
		sprintf(buf, "%dx%d", mode.x, mode.y);
		modeSelect->add(buf);
	}
	sprintf(buf, "%dx%d", (int)r_videoWidth, (int)r_videoHeight);
	modeSelect->value(buf);
	
	msaaSelect->clear();
	msaaSelect->add("Off");
	msaaSelect->add("MSAA 2x");
	msaaSelect->add("MSAA 4x");
	msaaSelect->add("FXAA");
	msaaSelect->add("Custom");
	if(r_fxaa) {
		if(r_multisamples){
			msaaSelect->value(4);
		}else{
			msaaSelect->value(3);
		}
	}else{
		switch((int)r_multisamples){
			case 0:
			case 1:
			default:
				msaaSelect->value(0);
				break;
			case 2:
				msaaSelect->value(1);
				break;
			case 4:
				msaaSelect->value(2);
				break;
		}
	}
	
	quickHostInput->value(cg_lastQuickConnectHost.CString());
	fullscreenCheck->value(r_fullscreen ? 1 : 0);
	
	// --- graphics
	if(r_cameraBlur && r_bloom && r_lens && r_lensFlare) {
		advancedLensCheck->value(1);
	}else{
		advancedLensCheck->value(0);
	}
	
	softParticleCheck->value(r_softParticles ? 1 : 0);
	radiosityCheck->value(r_radiosity ? 1 : 0);
	bloodCheck->value(cg_blood ? 1 : 0);
	
	directLightSelect->clear();
	directLightSelect->add("Low");
	directLightSelect->add("Medium");
	directLightSelect->add("High");
	directLightSelect->add("Custom");
	
	if((!r_mapSoftShadow) && (!r_dlights) && (!r_modelShadows) && (!r_fogShadow)){
		directLightSelect->value(0);
	}else if((!r_mapSoftShadow) && (r_dlights) && (r_modelShadows) && (!r_fogShadow)){
		directLightSelect->value(1);
	}else if((r_mapSoftShadow) && (r_dlights) && (r_modelShadows) && (r_fogShadow)){
		directLightSelect->value(2);
	}else{
		directLightSelect->value(3);
	}
	
	shaderSelect->clear();
	shaderSelect->add("Low");
	shaderSelect->add("High");
	shaderSelect->add("Custom");
	
	if((!r_water)){
		shaderSelect->value(0);
	}else if((r_water)){
		shaderSelect->value(1);
	}else{
		shaderSelect->value(2);
	}
	
	// --- audio
	polyInput->step(16.);
	polyInput->range(32., 256.);
	polyInput->value((int)s_maxPolyphonics);
	
	eaxCheck->value(s_eax ? 1 : 0);
	
	// --- game
	playerNameInput->value(cg_playerName.CString());
	playerNameInput->maximum_size(15);
	
}

void MainWindow::Init() {
	SPADES_MARK_FUNCTION();
	
	
	// banner
	std::string data = spades::FileManager::ReadAllBytes("Gfx/Banner.png");
	Fl_PNG_Image *img = new Fl_PNG_Image("Gfx/Banner.png", (const unsigned char *)data.data(), data.size());
	bannerBox->image(img);
	
	
	
	// --- about
	std::string text, pkg;
#ifdef PACKAGE_STRING
	pkg = PACKAGE_STRING;
#else
	pkg = "OpenSpades [Unknown Version]";
#endif
	text = std::string((const char *)aboutText, sizeof(aboutText));
	text = spades::Replace(text, "${PACKAGE_STRING}",
						   pkg);
	
	aboutView->value(text.c_str());
	
	mainTab->value(groupAbout);
	
}

/** This function is called after showing window.
 * Creating SDL window before showing MainWindow results in
 * internal error on Mac OS X. */
void MainWindow::CheckGLCapability() {
	SPADES_MARK_FUNCTION();
	
	// check GL capabilities
	
	SPLog("Initializing SDL for capability query");
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN |
									 SDL_DOUBLEBUF);
	if(modes && modes != (SDL_Rect **)-1){
		g_modes.clear();
		for(size_t i = 0; modes[i]; i++){
			SDL_Rect mode = *(modes[i]);
			if(mode.w < 800 || mode.h < 600)
				continue;
			g_modes.push_back(spades::IntVector3::Make(mode.w, mode.h, 0));
			SPLog("Video Mode Found: %dx%d", mode.w, mode.h);
		}
		
	}
	
	bool capable = true;
	std::string msg;
	if(!SDL_SetVideoMode(1,1, 32, SDL_OPENGL|SDL_NOFRAME)){
		// OpenGL initialization failed!
		outputGLRenderer->value("N/A");
		outputGLVersion->value("N/A");
		outputGLSLVersion->value("N/A");
		
		std::string err = SDL_GetError();
		SPLog("SDL_SetVideoMode failed: %s", err.c_str());
		msg = "<b>OpenGL/SDL couldn't be initialized. "
						  "You cannot play OpenSpades.</b><br><br>"
						  "<b>Message from SDL</b><br>"
		+ err;
		capable = false;
	}else{
		
		const char *str;
		SPLog("--- OpenGL Renderer Info ---");
		if((str = (const char*)glGetString(GL_VENDOR)) != NULL) {
			SPLog("Vendor: %s", str);
		}
		if((str = (const char *)glGetString(GL_RENDERER)) != NULL) {
			outputGLRenderer->value(str);
			SPLog("Name: %s", str);
		}else{
			outputGLRenderer->value("(unknown)");
		}
		if((str = (const char *)glGetString(GL_VERSION)) != NULL) {
			outputGLVersion->value(str);
			SPLog("Version: %s", str);
		}else{
			outputGLVersion->value("(unknown)");
		}
		if((str = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION)) != NULL) {
			outputGLSLVersion->value(str);
			SPLog("Shading Language Version: %s", str);
		}else{
			outputGLSLVersion->value("(unknown)");
		}
		
		str = (const char*)glGetString(GL_EXTENSIONS);
		std::string extensions;
		if(str)
			extensions = str;
		const char * const requiredExtensions[] = {
			"GL_ARB_multitexture",
			"GL_ARB_shader_objects",
			"GL_ARB_shading_language_100",
			"GL_ARB_vertex_buffer_object",
			"GL_EXT_framebuffer_object",
			"GL_EXT_framebuffer_blit",
			NULL
		};
		
		SPLog("--- Extensions ---");
		std::vector<std::string> strs = spades::Split(str, " ");
		for(size_t i = 0; i < strs.size(); i++)
			SPLog("%s", strs[i].c_str());
		SPLog("------------------");
		
		for(size_t i = 0; requiredExtensions[i]; i++) {
			const char *ex = requiredExtensions[i];
			if(extensions.find(ex) == std::string::npos) {
				// extension not found
				msg += "<font color=#ff0000>";
				msg += ex;
				msg += " NOT SUPPORTED!";
				msg += "</font>";
				capable = false;
			}else{
				msg += "<font color=#007f00>";
				msg += ex;
				msg += " is supported";
				msg += "</font>";
			}
			msg += "<br>";
		}
		
		// non-requred extensions
		if(extensions.find("GL_ARB_framebuffer_sRGB") ==
		   std::string::npos) {
			if(r_srgb) {
				r_srgb = 0;
				SPLog("Disabling r_srgb: no GL_ARB_framebuffer_sRGB");
			}
		}
		
		if(capable){
			msg = "Your video card supports all "
			"required OpenGL extensions.<br>" + msg;
		}else{
			msg = "<b>Your video card/driver doesn't support "
			"at least one of required OpenGL extensions."
			 " You cannot play OpenSpades.</b><br>" + msg;
		}
		
	}
	msg = "<font face=Helvetica>" + msg + "</font><a name=last></a>";
	
	glInfoView->value(msg.c_str());
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SPLog("SDL video subsystem finalized");
	
	SPLog("System is OpenSpades capable: %s",
		  capable ? "YES": "NO");
	if(!capable) {
		mainTab->value(groupReport);
		connectButton->deactivate();
	}
	
	LoadPrefs();
	
	inited = true;
}

void MainWindow::SavePrefs() {
	SPADES_MARK_FUNCTION();
	if(!inited)
		return;
	
	std::string modeStr = modeSelect->value();
	size_t pos = modeStr.find('x');
	if(pos != std::string::npos){
		int w = atoi(modeStr.substr(0, pos).c_str());
		int h = atoi(modeStr.substr(pos + 1).c_str());
		if(w >= 256 && h >= 256){
			r_videoWidth = w;
			r_videoHeight = h;
		}
	}
	
	cg_lastQuickConnectHost = quickHostInput->value();
	r_fullscreen = fullscreenCheck->value() ? 1 : 0;
	switch(msaaSelect->value()){
		case 0: r_multisamples = 0; r_fxaa = 0; break;
		case 1: r_multisamples = 2; r_fxaa = 0; break;
		case 2: r_multisamples = 4; r_fxaa = 0; break;
		case 3: r_multisamples = 0; r_fxaa = 1; break;
	}
	
	// --- graphics
	cg_blood = bloodCheck->value() ? 1 : 0;
	r_bloom = advancedLensCheck->value() ? 1 : 0;
	r_lens = advancedLensCheck->value() ? 1 : 0;
	r_lensFlare = advancedLensCheck->value() ? 1 : 0;
	r_cameraBlur = advancedLensCheck->value() ? 1 : 0;
	r_softParticles = softParticleCheck->value() ? 1 : 0;
	r_radiosity = radiosityCheck->value() ? 1 : 0;
	switch(directLightSelect->value()){
		case 0:
			r_modelShadows = 0;
			r_dlights = 0;
			r_mapSoftShadow = 0;
			r_fogShadow = 0;
			break;
		case 1:
			r_modelShadows = 1;
			r_dlights = 1;
			r_mapSoftShadow = 0;
			r_fogShadow = 0;
			break;
		case 2:
			r_modelShadows = 1;
			r_dlights = 1;
			r_mapSoftShadow = 1;
			r_fogShadow = 1;
			break;
	}
	switch(shaderSelect->value()){
		case 0:
			r_water = 0;
			break;
		case 1:
			r_water = 1;
			break;
	}
	
	// --- audio
	s_maxPolyphonics = (int)polyInput->value();
	s_eax = eaxCheck->value() ? 1 : 0;
	
	// --- game
	cg_playerName = playerNameInput->value();
	
}

void MainWindow::DisableMSAA() {
	if(msaaSelect->value() >= 1 && msaaSelect->value() <= 2)
		msaaSelect->value(3);
}

void MainWindow::MSAAEnabled() {
	if(shaderSelect->value() == 1)
		shaderSelect->value(0);
	if(directLightSelect->value() == 2)
		directLightSelect->value(1);
}

void MainWindow::OpenDetailConfig() {
	SPADES_MARK_FUNCTION();
	
	DetailConfigWindow cfg;
	cfg.set_modal();
	cfg.Init();
	cfg.show();
	while(cfg.visible()){
		Fl::wait();
	}
	LoadPrefs();
}



