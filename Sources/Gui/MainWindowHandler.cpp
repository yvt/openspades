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
#include "Main.h"

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
#include "Serverbrowser.h"
#include "ErrorDialog.h"

#include "../Imports/OpenGL.h" //for gpu info

using namespace spades::gui;

SPADES_SETTING(cg_smp, "0");
SPADES_SETTING(cg_blood, "1");
SPADES_SETTING(cg_lastQuickConnectHost, "127.0.0.1");
SPADES_SETTING(cg_protocolVersion, "");
SPADES_SETTING(cg_playerName, "Deuce");
SPADES_SETTING(r_bloom, "0");
SPADES_SETTING(r_lens, "1");
SPADES_SETTING(r_cameraBlur, "1");
SPADES_SETTING(r_softParticles, "1");
SPADES_SETTING(r_mapSoftShadow, "0");
SPADES_SETTING(r_modelShadows, "1");
SPADES_SETTING(r_radiosity, "0");
SPADES_SETTING(r_dlights, "1");
SPADES_SETTING(r_water, "2");
SPADES_SETTING(r_multisamples, "0");
SPADES_SETTING(r_fxaa, "1");
SPADES_SETTING(r_depthBits, "");
SPADES_SETTING(r_colorBits, "");
SPADES_SETTING(r_videoWidth, "1024");
SPADES_SETTING(r_videoHeight, "640");
SPADES_SETTING(r_fullscreen, "0");
SPADES_SETTING(r_fogShadow, "0");
SPADES_SETTING(r_lensFlare, "1");
SPADES_SETTING(r_lensFlareDynamic, "");
SPADES_SETTING(r_blitFramebuffer, "1");
SPADES_SETTING(r_srgb, "");
SPADES_SETTING(r_shadowMapSize, "");
SPADES_SETTING(s_maxPolyphonics, "96");
SPADES_SETTING(s_eax, "1");
SPADES_SETTING(r_maxAnisotropy, "8");
SPADES_SETTING(r_colorCorrection, "1");
SPADES_SETTING(r_physicalLighting, "");
SPADES_SETTING(r_occlusionQuery, "");
SPADES_SETTING(r_depthOfField, "");
SPADES_SETTING(r_vsync, "");
SPADES_SETTING(r_renderer, "");
SPADES_SETTING(r_swUndersampling, "");
SPADES_SETTING(s_audioDriver, "");

SPADES_SETTING(cl_showStartupWindow, "-1");

static std::vector<spades::IntVector3> g_modes;

MainWindow::~MainWindow()
{
	SPADES_MARK_FUNCTION();
	if( browser ) {
		delete browser;
	}
}

void MainWindow::QuickConnectPressed() {
	SPADES_MARK_FUNCTION();
	spades::ServerAddress host(quickHostInput->value(), versionChoice->value() == 0 ? spades::ProtocolVersion::v075 : spades::ProtocolVersion::v076);
	hide();
	spades::StartClient(host, cg_playerName);
}

void MainWindow::StartGamePressed() {
	SPADES_MARK_FUNCTION();
	
	if((int)cl_showStartupWindow == -1){
		SPLog("This seems to be the first launch of the current version (cl_showStartupWindow = -1).");
		switch(fl_choice("Do you want to bypass this startup window the next time you launch OpenSpades?\n\n"
						 "Note: You can access the startup window again by holding the Shift key.",
						 "No", "Yes", "Cancel")) {
			case 1:
				SPLog("User wants to bypass the startup window.");
				SPLog("Setting cl_showStartupWindow to 0");
				cl_showStartupWindow = 0;
				break;
			case 0:
				SPLog("User don't want to bypass the startup window.");
				SPLog("Setting cl_showStartupWindow to 1");
				cl_showStartupWindow = 1;
				break;
			case 2:
				return;
		}
	}
	
	
	hide();
	SPLog("Starting main screen");
	spades::StartMainScreen();
}

void MainWindow::connectLocal075Pressed()
{
	SPADES_MARK_FUNCTION();
	spades::ServerAddress host("aos://16777343:32887", spades::ProtocolVersion::v075 );
	hide();
	spades::StartClient(host, cg_playerName);
}

void MainWindow::connectLocal076Pressed()
{
	SPADES_MARK_FUNCTION();
	spades::ServerAddress host("aos://16777343:32887", spades::ProtocolVersion::v076 );
	hide();
	spades::StartClient(host, cg_playerName);
}

void MainWindow::versionSelectionChanged()
{
	SPADES_MARK_FUNCTION();
	cg_protocolVersion = versionChoice->value() + 3;
}

#pragma mark - Setup

void MainWindow::LoadPrefs() {
	SPADES_MARK_FUNCTION();
	
	SPLog("Loading Preferences to MainWindow");
	
	switch((int)cl_showStartupWindow) {
		case -1:
		case 0:
			bypassStartupCheck->value(1);
			break;
		default:
			bypassStartupCheck->value(0);
			break;
	}
	
	
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
	if(r_blitFramebuffer) {
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
	}else {
		// MSAA is not supported with r_blitFramebuffer = 0
		msaaSelect->add("Off");
		msaaSelect->add("FXAA");
		msaaSelect->add("Custom");
		if(r_fxaa) {
			if(r_multisamples){
				msaaSelect->value(2);
			}else{
				msaaSelect->value(1);
			}
		}else{
			switch((int)r_multisamples){
				case 0:
				case 1:
				default:
					msaaSelect->value(0);
					break;
				case 2:
				case 4:
					msaaSelect->value(2);
					break;
			}
		}
	}
	
	quickHostInput->value(cg_lastQuickConnectHost.CString());
	int v = (int)cg_protocolVersion;
	versionChoice->add( "0.75" );
	versionChoice->add( "0.76" );
	versionChoice->value( v == 3 ? 0 : 1 ); 
	fullscreenCheck->value(r_fullscreen ? 1 : 0);
	verticalSyncCheck->value(r_vsync ? 1 : 0);
	
	// --- graphics
	swRendererGroup->hide();
	glRendererGroup->hide();
	rendererSelect->clear();
	rendererSelect->add("Software");
	if(glCapable)
		rendererSelect->add("OpenGL");
	if(spades::EqualsIgnoringCase(r_renderer, "sw")) {
		swRendererGroup->show();
		rendererSelect->value(0);
	}else if(spades::EqualsIgnoringCase(r_renderer, "gl")) {
		glRendererGroup->show();
		rendererSelect->value(1);
	}
	
	// --- GL graphics
	postFilterSelect->clear();
	postFilterSelect->add("Low");
	postFilterSelect->add("Medium");
	if(postFilterHighCapable){
		postFilterSelect->add("High");
	}
	postFilterSelect->add("Custom");
	if(postFilterHighCapable){
		postFilterSelect->value(3);
	}else{
		postFilterSelect->value(2);
	}
	
	if(r_cameraBlur && r_bloom && r_lens && r_lensFlare && r_lensFlareDynamic &&
	   r_colorCorrection && r_depthOfField
	   && postFilterHighCapable) {
		postFilterSelect->value(2);
	}else if(r_cameraBlur && (!r_bloom) && r_lens && r_lensFlare && (!r_lensFlareDynamic) &&
	   r_colorCorrection && (!r_depthOfField)) {
		postFilterSelect->value(1);
	}else if((!r_cameraBlur) && (!r_bloom) && (!r_lens) && (!r_lensFlare) && (!r_lensFlareDynamic) &&
			 (!r_colorCorrection) && (!r_depthOfField)) {
		postFilterSelect->value(0);
	}
	
	particleSelect->clear();
	particleSelect->add("Low");
	particleSelect->add("Medium");
	if(particleHighCapable) {
		particleSelect->add("High");
	}
	particleSelect->add("Custom");
	particleSelect->value(particleHighCapable ? 3 : 2);
	if((int)r_softParticles >= 2 && particleHighCapable) {
		particleSelect->value(2);
	}else if((int)r_softParticles == 1) {
		particleSelect->value(1);
	}else if((int)r_softParticles == 0){
		particleSelect->value(0);
	}
	radiosityCheck->value(r_radiosity ? 1 : 0);
	bloodCheck->value(cg_blood ? 1 : 0);
	
	directLightSelect->clear();
	directLightSelect->add("Low");
	directLightSelect->add("Medium");
	directLightSelect->add("High");
	directLightSelect->add("Ultra");
	directLightSelect->add("Custom");
	
	if((!r_mapSoftShadow) && (r_dlights) && (!r_modelShadows) && (!r_fogShadow) && (!r_physicalLighting)){
		directLightSelect->value(0);
	}else if((!r_mapSoftShadow) && (r_dlights) && (r_modelShadows) && (!r_fogShadow) && (!r_physicalLighting)){
		directLightSelect->value(1);
	}else if((r_mapSoftShadow) && (r_dlights) && (r_modelShadows) && (!r_fogShadow) && (r_physicalLighting)){
		directLightSelect->value(2);
	}else if((r_mapSoftShadow) && (r_dlights) && (r_modelShadows) && (r_fogShadow) && (r_physicalLighting)){
		directLightSelect->value(3);
	}else{
		directLightSelect->value(4);
	}
	
	shaderSelect->clear();
	shaderSelect->add("Low");
	shaderSelect->add("Medium");
	if(shaderHighCapable){
		shaderSelect->add("High");
	}
	shaderSelect->add("Custom");
	
	if(shaderHighCapable){
		if((!r_water)){
			shaderSelect->value(0);
		}else if((int)r_water == 1){
			shaderSelect->value(1);
		}else if((int)r_water == 2){
			shaderSelect->value(2);
		}else{
			shaderSelect->value(3);
		}
	}else{
		if((!r_water)){
			shaderSelect->value(0);
		}else if((int)r_water == 1){
			shaderSelect->value(1);
		}else{
			shaderSelect->value(2);
		}
	}
	
	// --- SW graphics
	fastModeSelect->clear();
	fastModeSelect->add("1x");
	fastModeSelect->add("2x");
	fastModeSelect->add("4x");
	switch((int)r_swUndersampling) {
		case 2:
			fastModeSelect->value(1);
			break;
		case 4:
			fastModeSelect->value(2);
			break;
		default:
			fastModeSelect->value(0);
			break;
	}
	
	// --- audio
	polyInput->step(16.);
	polyInput->range(32., 256.);
	polyInput->value((int)s_maxPolyphonics);
	
	audioSelect->clear();
	audioSelect->add("OpenAL");
	audioSelect->add("OpenAL + EAX");
	audioSelect->add("YSR");
	if(spades::EqualsIgnoringCase(s_audioDriver, "ysr")) {
		audioSelect->value(2);
	}else{
		audioSelect->value(s_eax ? 1 : 0);
	}
	
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
	pkg = PACKAGE_STRING;
	text = std::string((const char *)aboutText, sizeof(aboutText));
	text = spades::Replace(text, "${PACKAGE_STRING}", pkg);

	aboutView->value(text.c_str());
	
	browser = new spades::Serverbrowser( serverListbox );
	spades::ServerFilter::Flags flags = browser->Filter();
	checkFilterEmpty->value( flags & spades::ServerFilter::flt_Empty );
	checkFilterFull->value( flags & spades::ServerFilter::flt_Full );
	checkFilterV75->value( flags & spades::ServerFilter::flt_Ver075 );
	checkFilterV76->value( flags & spades::ServerFilter::flt_Ver076 );
	checkFilterVOther->value( flags & spades::ServerFilter::flt_VerOther );
	//browser->startQuery();
	//mainTab->value(groupServerlist);
	//groupServerlist->value(serverListbox);
	
	mainTab->value(groupAbout);
	groupServerlist->hide();
	mainTab->remove(3);
}

/** This function is called after showing window.
 * Creating SDL window before showing MainWindow results in
 * internal error on Mac OS X. */
void MainWindow::CheckGLCapability() {
	SPADES_MARK_FUNCTION();
	
	// check GL capabilities
	
	SPLog("Initializing SDL for capability query");
	SDL_Init(SDL_INIT_VIDEO);
	
	int idDisplay = 0;
	
	int numDisplayMode = SDL_GetNumDisplayModes(idDisplay);
	SDL_DisplayMode mode;
	g_modes.clear();
	if(numDisplayMode > 0){
		for(int i = 0; i < numDisplayMode; i++) {
			SDL_GetDisplayMode(idDisplay, i, &mode);
			if(mode.w < 800 || mode.h < 600)
				continue;
			g_modes.push_back(spades::IntVector3::Make(mode.w, mode.h, 0));
			SPLog("Video Mode Found: %dx%d", mode.w, mode.h);
		}
	}else{
		SPLog("Failed to get video mode list. Presetting default list");
		g_modes.push_back(spades::IntVector3::Make(800, 600, 0));
		g_modes.push_back(spades::IntVector3::Make(1024, 768, 0));
		g_modes.push_back(spades::IntVector3::Make(1280, 720, 0));
		g_modes.push_back(spades::IntVector3::Make(1920, 1080, 0));
	}
	
	
	bool capable = true;
	std::string msg;
	SDL_Window *window = SDL_CreateWindow("Querying OpenGL Capabilities",
									   1, 1, 1, 1,
									   SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
	if(window == nullptr) {
		SPLog("Failed to create SDL window: %s", SDL_GetError());
	}
	SDL_GLContext context = window ? SDL_GL_CreateContext(window) : nullptr;
	if(window != nullptr && context == nullptr) {
		SPLog("Failed to create OpenGL context: %s", SDL_GetError());
	}
	if(!context){
		// OpenGL initialization failed!
		outputGLRenderer->value("N/A");
		outputGLVersion->value("N/A");
		outputGLSLVersion->value("N/A");
		
		std::string err = SDL_GetError();
		SPLog("SDL_SetVideoMode failed: %s", err.c_str());
		msg = "<b>OpenGL/SDL couldn't be initialized. "
						  "Falling back to the software renderer.</b><br><br>"
						  "<b>Message from SDL</b><br>"
		+ err;
		capable = false;
		shaderHighCapable = false;
		postFilterHighCapable = false;
		particleHighCapable = false;
	}else{
		
		SDL_GL_MakeCurrent(window, context);
		
		shaderHighCapable = true;
		postFilterHighCapable = true;
		particleHighCapable = true;
		
		const char *str;
		GLint maxTextureSize;
		GLint max3DTextureSize;
		GLint maxCombinedTextureUnits;
		GLint maxVertexTextureUnits;
		GLint maxVaryingComponents;
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
			double ver = atof(str);
			if( ver <= 0.1 ) {		//TODO: determine required version!
				std::string tmp = str;
				tmp += "  (too old)";
				outputGLVersion->textcolor( FL_RED );
				outputGLVersion->value( tmp.c_str() );
				capable = false;
			}else{
				outputGLVersion->value( str );
			}
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
		
		maxTextureSize = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
		if(maxTextureSize > 0) {
			SPLog("Max Texture Size: %d", (int)maxTextureSize);
		}
		max3DTextureSize = 0;
		glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTextureSize);
		if(max3DTextureSize > 0) {
			SPLog("Max 3D Texture Size: %d", (int)max3DTextureSize);
		}
		
		maxCombinedTextureUnits = 0;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);
		if(maxCombinedTextureUnits > 0) {
			SPLog("Max Combined Texture Image Units: %d", (int)maxCombinedTextureUnits);
		}
		
		maxVertexTextureUnits = 0;
		glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureUnits);
		if(maxVertexTextureUnits > 0) {
			SPLog("Max Vertex Texture Image Units: %d", (int)maxVertexTextureUnits);
		}
		
		maxVaryingComponents = 0;
		glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &maxVaryingComponents);
		if(maxVaryingComponents > 0) {
			SPLog("Max Varying Components: %d", (int)maxVaryingComponents);
		}
		
		str = (const char*)glGetString(GL_EXTENSIONS);
		std::string extensions;
		if(str)
			extensions = str;
		const char * const requiredExtensions[] = {
			"GL_ARB_multitexture",
			"GL_ARB_shader_objects",
			"GL_ARB_shading_language_100",
			"GL_ARB_texture_non_power_of_two",
			"GL_ARB_vertex_buffer_object",
			"GL_EXT_framebuffer_object",
			NULL
		};
		
		SPLog("--- Extensions ---");
		std::vector<std::string> strs = spades::Split(str, " ");
		for(size_t i = 0; i < strs.size(); i++) {
			SPLog("%s", strs[i].c_str());
		}
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
		
		msg += "<br>&nbsp;<br>";
		msg += "<b>Other Extensions:</b><br>";
		
		// non-requred extensions
		if(extensions.find("GL_ARB_framebuffer_sRGB") ==
		   std::string::npos) {
			if(r_srgb) {
				r_srgb = 0;
				SPLog("Disabling r_srgb: no GL_ARB_framebuffer_sRGB");
			}
			msg += "GL_ARB_framebuffer_sRGB is NOT SUPPORTED<br>";
			msg += "&nbsp;&nbsp;r_srgb is disabled.<br>";
		}else{
			msg += "<font color=#007f00>";
			msg += "GL_ARB_framebuffer_sRGB is supported";
			msg += "</font><br>";
		}
		if(extensions.find("GL_EXT_framebuffer_blit") ==
		   std::string::npos) {
			if(r_blitFramebuffer) {
				r_blitFramebuffer = 0;
				SPLog("Disabling r_blitFramebuffer: no GL_EXT_framebuffer_blit");
			}
			if(r_multisamples) {
				r_multisamples = 0;
				SPLog("Disabling r_multisamples: no GL_EXT_framebuffer_blit");
			}
			msg += "GL_EXT_framebuffer_blit is NOT SUPPORTED<br>";
			msg += "&nbsp;&nbsp;MSAA is disabled.<br>";
			msg += "&nbsp;&nbsp;r_blitFramebuffer is disabled.<br>";
		}else{
			msg += "<font color=#007f00>";
			msg += "GL_EXT_framebuffer_blit is supported";
			msg += "</font><br>";
		}
		if(extensions.find("GL_EXT_texture_filter_anisotropic") ==
		   std::string::npos) {
			if((float)r_maxAnisotropy > 1.1f) {
				r_maxAnisotropy = 1;
				SPLog("Setting r_maxAnisotropy to 1: no GL_EXT_texture_filter_anisotropic");
			}
			msg += "GL_EXT_texture_filter_anisotropic is NOT SUPPORTED<br>";
			msg += "&nbsp;&nbsp;r_maxAnisotropy is disabled.<br>";
		}else{
			msg += "<font color=#007f00>";
			msg += "GL_EXT_texture_filter_anisotropic is supported";
			msg += "</font><br>";
		}
		
		if(extensions.find("GL_ARB_occlusion_query") ==
		   std::string::npos) {
			if(r_occlusionQuery) {
				r_occlusionQuery = 0;
				SPLog("Disabling r_occlusionQuery: no GL_ARB_occlusion_query");
			}
			msg += "GL_ARB_occlusion_query is NOT SUPPORTED<br>";
			msg += "&nbsp;&nbsp;r_occlusionQuery is disabled<br>";
		}else{
			msg += "<font color=#007f00>";
			msg += "GL_ARB_occlusion_query is supported";
			msg += "</font><br>";
		}
		
		if(extensions.find("GL_NV_conditional_render") ==
		   std::string::npos) {
			if(r_occlusionQuery) {
				r_occlusionQuery = 0;
				SPLog("Disabling r_occlusionQuery: no GL_NV_conditional_render");
			}
			msg += "GL_NV_conditional_render is NOT SUPPORTED<br>";
			msg += "&nbsp;&nbsp;r_occlusionQuery is disabled<br>";
		}else{
			msg += "<font color=#007f00>";
			msg += "GL_NV_conditional_render is supported";
			msg += "</font><br>";
		}
		
		msg += "<br>&nbsp;<br>";
		msg += "<b>Miscellaneous:</b><br>";
		char buf[256];
		sprintf(buf, "Max Texture Size: %d<br>", (int)maxTextureSize);
		msg += buf;
		if(maxTextureSize < 1024) {
			capable = false;
			msg += "<font color=#ff0000>";
			msg += "&nbsp;&nbsp;TOO SMALL (1024 required)";
			msg += "</font><br>";
		}
		if((int)r_shadowMapSize > maxTextureSize) {
			SPLog("Changed r_shadowMapSize from %d to %d: too small GL_MAX_TEXTURE_SIZE", (int)r_shadowMapSize, maxTextureSize);
			
			r_shadowMapSize = maxTextureSize;
		}
		
		sprintf(buf, "Max 3D Texture Size: %d<br>", (int)max3DTextureSize);
		msg += buf;
		if(max3DTextureSize < 512) {
			msg += "&nbsp;&nbsp;Global Illumation is disabled (512 required)<br>";
			
			if(r_radiosity) {
				r_radiosity = 0;
				SPLog("Disabling r_radiosity: too small GL_MAX_3D_TEXTURE_SIZE");
				
				radiosityCheck->deactivate();
			}
		}
		
		
		sprintf(buf, "Max Combined Texture Image Units: %d<br>", (int)maxCombinedTextureUnits);
		msg += buf;
		if(maxCombinedTextureUnits < 12) {
			msg += "&nbsp;&nbsp;Global Illumation is disabled (12 required)<br>";
			
			if(r_radiosity) {
				r_radiosity = 0;
				SPLog("Disabling r_radiosity: too small GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS");
				
				radiosityCheck->deactivate();
			}
		}

		sprintf(buf, "Max Vertex Texture Image Units: %d<br>", (int)maxVertexTextureUnits);
		msg += buf;
		if(maxVertexTextureUnits < 3) {
			msg += "&nbsp;&nbsp;Water 2 is disabled (3 required)<br>";
			msg += "&nbsp;&nbsp;(Shader Effects is limited to Medium)<br>";
			shaderHighCapable = false;
			
			if((int)r_water >= 2) {
				r_water = 1;
				SPLog("Disabling Water 2: too small GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS");
			}
		}
		
		sprintf(buf, "Max Varying Components: %d<br>", (int)maxVaryingComponents);
		msg += buf;
		if(maxVaryingComponents < 37) {
			msg += "&nbsp;&nbsp;Shaded Particle is disabled (37 required)<br>";
			msg += "&nbsp;&nbsp;(Particle is limited to Medium)<br>";
			particleHighCapable = false;
			
			if((int)r_softParticles >= 2) {
				r_softParticles = 1;
				SPLog("Disabling Water 2: too small GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS");
			}
		}
		
		
		
		if(capable){
			msg = "Your video card supports all "
			"required OpenGL extensions/features.<br>&nbsp;<br>" + msg;
		}else{
			msg = "<b>Your video card/driver doesn't support "
			"at least one of required OpenGL extensions/features."
			 " Falling back to the software renderer.</b><br>&nbsp;<br>" + msg;
		}
		
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		
		SPLog("SDL Capability Query Window finalized");
		
	}
	msg = "<font face=Helvetica>" + msg + "</font><a name=last></a>";
	
	glInfoView->value(msg.c_str());
	
	
	SPLog("OpenGL driver is OpenSpades capable: %s",
		  capable ? "YES": "NO");
	glCapable = capable;
	if(!glCapable) {
		if(spades::EqualsIgnoringCase(r_renderer, "gl")){
			SPLog("Switched to software renderer");
			r_renderer = "sw";
		}
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
	
	r_vsync = verticalSyncCheck->value() ? 1 : 0;
	
	if((int)cl_showStartupWindow == -1){
		if(!bypassStartupCheck->value()) {
			cl_showStartupWindow = 1;
		}
	}else{
		cl_showStartupWindow = bypassStartupCheck->value() ? 0 : 1;
	}
	
	cg_lastQuickConnectHost = quickHostInput->value();
	cg_protocolVersion = versionChoice->value() + 3;	//0  = 3 = 0.75, 1 = 4 = 0.76
	r_fullscreen = fullscreenCheck->value() ? 1 : 0;
	switch(msaaSelect->value()){
		case 0: r_multisamples = 0; r_fxaa = 0; break;
		case 1: r_multisamples = 2; r_fxaa = 0; break;
		case 2: r_multisamples = 4; r_fxaa = 0; break;
		case 3: r_multisamples = 0; r_fxaa = 1; break;
	}
	
	// --- graphics
	switch(rendererSelect->value()) {
		case 1:
			r_renderer = "gl";
			break;
		case 0:
			r_renderer = "sw";
			break;
	}
	cg_blood = bloodCheck->value() ? 1 : 0;
	switch(postFilterSelect->value()){
		case 0:
			r_bloom = 0;
			r_lens = 0;
			r_lensFlare = 0;
			r_lensFlareDynamic = 0;
			r_cameraBlur = 0;
			r_colorCorrection = 0;
			r_depthOfField = 0;
			break;
		case 1:
			r_bloom = 0;
			r_lens = 1;
			r_lensFlare = 1;
			r_lensFlareDynamic = 0;
			r_cameraBlur = 1;
			r_colorCorrection = 1;
			r_depthOfField = 0;
			break;
		case 2:
			if(postFilterHighCapable){
				r_bloom = 1;
				r_lens = 1;
				r_lensFlare = 1;
				r_lensFlareDynamic = 1;
				r_cameraBlur = 1;
				r_colorCorrection = 1;
				r_depthOfField = 1;
			}
			break;
	}
	switch(particleSelect->value()){
		case 0:
			r_softParticles = 0;
			break;
		case 1:
			r_softParticles = 1;
			break;
		case 2:
			if(particleHighCapable){
				r_softParticles = 2;
			}
			break;
	}
	r_radiosity = radiosityCheck->value() ? 1 : 0;
	switch(directLightSelect->value()){
		case 0:
			r_modelShadows = 0;
			r_dlights = 1;
			r_mapSoftShadow = 0;
			r_fogShadow = 0;
			r_physicalLighting = 0;
			break;
		case 1:
			r_modelShadows = 1;
			r_dlights = 1;
			r_mapSoftShadow = 0;
			r_fogShadow = 0;
			r_physicalLighting = 0;
			break;
		case 2:
			r_modelShadows = 1;
			r_dlights = 1;
			r_mapSoftShadow = 1;
			r_fogShadow = 0;
			r_physicalLighting = 1;
			break;
		case 3:
			r_modelShadows = 1;
			r_dlights = 1;
			r_mapSoftShadow = 1;
			r_fogShadow = 1;
			r_physicalLighting = 1;
			break;
	}
	if(shaderHighCapable){
		switch(shaderSelect->value()){
			case 0:
				r_water = 0;
				break;
			case 1:
				r_water = 1;
				break;
			case 2:
				r_water = 2;
				break;
		}
	}else{
		switch(shaderSelect->value()){
			case 0:
				r_water = 0;
				break;
			case 1:
				r_water = 1;
				break;
		}
	}
	switch(fastModeSelect->value()){
		case 0:
			r_swUndersampling = 1;
			break;
		case 1:
			r_swUndersampling = 2;
			break;
		case 2:
			r_swUndersampling = 4;
			break;
	}
	
	// --- audio
	s_maxPolyphonics = (int)polyInput->value();
	switch(audioSelect->value()){
		case 0:
			s_audioDriver = "openal";
			s_eax = 0;
			break;
		case 1:
			s_audioDriver = "openal";
			s_eax = 1;
			break;
		case 2:
			s_audioDriver = "ysr";
			s_eax = 1;
			break;
	}
	
	// --- game
	cg_playerName = playerNameInput->value();
	
	swRendererGroup->hide();
	glRendererGroup->hide();
	if(spades::EqualsIgnoringCase(r_renderer, "sw")) {
		swRendererGroup->show();
		rendererSelect->value(0);
	}else if(spades::EqualsIgnoringCase(r_renderer, "gl")) {
		glRendererGroup->show();
		rendererSelect->value(1);
	}
}

void MainWindow::DisableMSAA() {
	if(r_blitFramebuffer){
		if(msaaSelect->value() >= 1 && msaaSelect->value() <= 2)
			msaaSelect->value(3);
	}
}

void MainWindow::MSAAEnabled() {
	if(msaaSelect->value() >= 1 &&
	   msaaSelect->value() <= 2 &&
	   r_blitFramebuffer){
		if(shaderSelect->value() == 1)
			shaderSelect->value(0);
		if(directLightSelect->value() == 3)
			directLightSelect->value(2);
	}
}

void MainWindow::OpenDetailConfig() {
	SPADES_MARK_FUNCTION();
	
	DetailConfigWindow cfg;
	cfg.icon( icon() );	//use the icon from the main window
	cfg.set_modal();
	cfg.show();
	while(cfg.visible()){
		Fl::wait();
	}
	LoadPrefs();
}

void MainWindow::ServerSelectionChanged()
{
	SPADES_MARK_FUNCTION();
	if( browser ) {
		int item = serverListbox->value();
		if( item > 1 ) {
			browser->onSelection( serverListbox->data( item ), quickHostInput, versionChoice );
		} else if( item == 1 ) {
			browser->onHeaderClick( Fl::event_x() - serverListbox->x() );
		}
	}	
}

void MainWindow::updateFilters()
{
	if( browser ) {
		spades::ServerFilter::Flags flags = spades::ServerFilter::flt_None;
		if( checkFilterEmpty->value() ) {
			flags |= spades::ServerFilter::flt_Empty;
		}
		if( checkFilterFull->value() ) {
			flags |= spades::ServerFilter::flt_Full;
		}
		if( checkFilterV75->value() ) {
			flags |= spades::ServerFilter::flt_Ver075;
		}
		if( checkFilterV76->value() ) {
			flags |= spades::ServerFilter::flt_Ver076;
		}
		if( checkFilterVOther->value() ) {
			flags |= spades::ServerFilter::flt_VerOther;
		}
		browser->setFilter( flags );
		browser->refreshList();
	}
}

