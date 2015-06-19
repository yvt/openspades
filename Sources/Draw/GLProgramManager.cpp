/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include "GLProgramManager.h"
#include "../Core/FileManager.h"
#include "../Core/Math.h"
#include "GLProgram.h"
#include "GLShader.h"
#include "../Core/Exception.h"
#include "../Core/Debug.h"
#include "GLShadowShader.h"
#include "GLDynamicLightShader.h"
#include "IGLShadowMapRenderer.h"
#include "GLShadowMapShader.h"
#include "../Core/Stopwatch.h"
#include <Core/Settings.h>

SPADES_SETTING(r_hdr, "");
SPADES_SETTING(r_fogShadow, "");

namespace spades {
	namespace draw {
		GLProgramManager::GLProgramManager(IGLDevice *d,
										   IGLShadowMapRenderer *smr):
		device(d), shadowMapRenderer(smr){
			SPADES_MARK_FUNCTION();
		}
		
		GLProgramManager::~GLProgramManager() {
			SPADES_MARK_FUNCTION();
			
			for(std::map<std::string, GLProgram *>::iterator it = programs.begin(); it != programs.end(); it++){
				delete it->second;
			}
			for(std::map<std::string, GLShader *>::iterator it = shaders.begin(); it != shaders.end(); it++){
				delete it->second;
			}
		}
		
		GLProgram *GLProgramManager::RegisterProgram(const std::string &name) {
			SPADES_MARK_FUNCTION();
			
			std::map<std::string, GLProgram *>::iterator it;
			it = programs.find(name);
			if(it == programs.end()){
				GLProgram *prog = CreateProgram(name);
				programs[name] = prog;
				return prog;
			}else{
				return it->second;
			}
		}
		
		GLShader *GLProgramManager::RegisterShader(const std::string &name) {
			SPADES_MARK_FUNCTION();
			
			std::map<std::string, GLShader *>::iterator it;
			it = shaders.find(name);
			if(it == shaders.end()){
				GLShader *prog = CreateShader(name);
				shaders[name] = prog;
				return prog;
			}else{
				return it->second;
			}
		}
		
		GLProgram *GLProgramManager::CreateProgram(const std::string &name) {
			SPADES_MARK_FUNCTION();
			
			SPLog("Loading GLSL program '%s'", name.c_str());
			std::string text = FileManager::ReadAllBytes(name.c_str());
			std::vector<std::string> lines = SplitIntoLines(text);
			
			GLProgram *p = new GLProgram(device, name);
			
			for(size_t i = 0; i < lines.size(); i++){
				std::string text = TrimSpaces(lines[i]);
				if(text.empty())
					break;
				
				if(text == "*shadow*"){
					std::vector<GLShader *> shaders = GLShadowShader::RegisterShader(this);
					for(size_t i = 0; i < shaders.size(); i++)
						p->Attach(shaders[i]);
					continue;
				}else if(text == "*shadow-variance*"){
					std::vector<GLShader *> shaders = GLShadowShader::RegisterShader(this, true);
					for(size_t i = 0; i < shaders.size(); i++)
						p->Attach(shaders[i]);
					continue;
				}else if(text == "*dlight*"){
					std::vector<GLShader *> shaders = GLDynamicLightShader::RegisterShader(this);
					for(size_t i = 0; i < shaders.size(); i++)
						p->Attach(shaders[i]);
					continue;
				}else if(text == "*shadowmap*"){
					std::vector<GLShader *> shaders = GLShadowMapShader::RegisterShader(this);
					for(size_t i = 0; i < shaders.size(); i++)
						p->Attach(shaders[i]);
					continue;
				}else if(text[0] == '*'){
					SPRaise("Unknown special shader: %s",text.c_str());
				}
				GLShader *s = CreateShader(text);
				
				p->Attach(s);
			}
			
			Stopwatch sw;
			p->Link();
			SPLog("Successfully linked GLSL program '%s' in %.3fms", name.c_str(), sw.GetTime() * 1000.);
			//p->Validate();
			return p;
		}
		
		GLShader *GLProgramManager::CreateShader(const std::string &name) {
			SPADES_MARK_FUNCTION();
			
			SPLog("Loading GLSL shader '%s'", name.c_str());
			std::string text = FileManager::ReadAllBytes(name.c_str());
			GLShader::Type type;
			
			if(name.find(".fs") != std::string::npos)
				type = GLShader::FragmentShader;
			else if(name.find(".vs") != std::string::npos)
				type = GLShader::VertexShader;
			else
				SPRaise("Failed to determine the type of a shader: %s",
						name.c_str());
			
			GLShader *s = new GLShader(device, type);
			
			std::string finalSource;
			
			if(r_hdr) {
				finalSource += "#define USE_HDR 1\n";
				finalSource += "#define LINEAR_FRAMEBUFFER 1\n";
			}else{
				finalSource += "#define USE_HDR 0\n";
				finalSource += "#define LINEAR_FRAMEBUFFER 0\n";
			}
			if(r_fogShadow) {
				finalSource += "#define USE_VOLUMETRIC_FOG 1\n";
			}else{
				finalSource += "#define USE_VOLUMETRIC_FOG 0\n";
			}
			
			finalSource += text;
			
			s->AddSource(finalSource);
			
			Stopwatch sw;
			s->Compile();
			SPLog("Successfully compiled GLSL program '%s' in %.3fms", name.c_str(),
				  sw.GetTime() * 1000.);
			return s;
		}
		
	}
}
