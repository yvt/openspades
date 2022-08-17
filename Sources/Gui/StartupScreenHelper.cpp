/*
 Copyright (c) 2013 yvt
 Portion of the code is based on Serverbrowser.cpp.

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

#include <algorithm>
#include <cctype>
#include <utility>
#include <regex>

#include <Imports/OpenGL.h> //for gpu info
#include <Imports/SDL.h>
#include <json/json.h>

#include "StartupScreenHelper.h"

#include "StartupScreen.h"
#include <Audio/ALDevice.h>
#include <Audio/YsrDevice.h>
#include <Core/FileManager.h>
#include <Core/Settings.h>
#include <OpenSpades.h>
#include <Core/ShellApi.h>
#include <Gui/Main.h>
#include <Gui/Icon.h>
#include <Gui/PackageUpdateManager.h>

SPADES_SETTING(r_bloom);
SPADES_SETTING(r_lens);
SPADES_SETTING(r_cameraBlur);
SPADES_SETTING(r_softParticles);
SPADES_SETTING(r_mapSoftShadow);
SPADES_SETTING(r_modelShadows);
SPADES_SETTING(r_radiosity);
SPADES_SETTING(r_dlights);
SPADES_SETTING(r_water);
SPADES_SETTING(r_multisamples);
SPADES_SETTING(r_fxaa);
SPADES_SETTING(r_videoWidth);
SPADES_SETTING(r_videoHeight);
SPADES_SETTING(r_fullscreen);
SPADES_SETTING(r_fogShadow);
SPADES_SETTING(r_lensFlare);
SPADES_SETTING(r_lensFlareDynamic);
SPADES_SETTING(r_blitFramebuffer);
SPADES_SETTING(r_srgb);
SPADES_SETTING(r_shadowMapSize);
SPADES_SETTING(s_maxPolyphonics);
SPADES_SETTING(s_eax);
SPADES_SETTING(r_maxAnisotropy);
SPADES_SETTING(r_colorCorrection);
SPADES_SETTING(r_physicalLighting);
SPADES_SETTING(r_occlusionQuery);
SPADES_SETTING(r_depthOfField);
SPADES_SETTING(r_vsync);
SPADES_SETTING(r_renderer);
SPADES_SETTING(r_swUndersampling);
SPADES_SETTING(r_hdr);
SPADES_SETTING(r_temporalAA);

namespace spades {
	namespace gui {

		StartupScreenHelper::StartupScreenHelper()
		    : scr(nullptr),
		      shaderHighCapable(false),
		      postFilterHighCapable(false),
		      particleHighCapable(false) {
			SPADES_MARK_FUNCTION();
		}

		StartupScreenHelper::~StartupScreenHelper() { SPADES_MARK_FUNCTION(); }

		void StartupScreenHelper::StartupScreenDestroyed() {
			SPADES_MARK_FUNCTION();
			scr = nullptr;
		}

		namespace {
			std::regex const localeInfoRegex("[-a-zA-Z0-9_]+\\.json");
		}

		void StartupScreenHelper::ExamineSystem() {
			SPADES_MARK_FUNCTION();

			// clear capability report
			// (this function can be called multiple times via StartupScreenHelper::FixConfig)
			reportLines.clear();
			report.clear();

			// check installed locales
			SPLog("Checking installed locales");

			auto localeDirectories = FileManager::EnumFiles("Locales");
			locales.clear();
			for (const std::string &localeInfoName : localeDirectories) {
				if (!std::regex_match(localeInfoName, localeInfoRegex)) {
					continue;
				}
				std::string locale = localeInfoName.substr(0, localeInfoName.size() - 5);
				std::string localeInfoPath = "Locales/" + localeInfoName;

				try {
					std::string buffer = FileManager::ReadAllBytes(localeInfoPath.c_str());
					LocaleInfo info;
					Json::Reader reader;
					Json::Value root;
					if (!reader.parse(buffer.c_str(), root, false)) {
						SPRaise("Failed to parse LocaleInfo.json: %s",
						        reader.getFormatedErrorMessages().c_str());
					}

					info.name = locale;
					info.descriptionEnglish = root["descriptionEnglish"].asString();
					info.descriptionNative = root["description"].asString();

					locales.push_back(std::move(info));

					SPLog("Locale '%s' found.", locale.c_str());
				} catch (const std::exception &ex) {
					SPLog("Error while reading the locale info for '%s': %s", locale.c_str(),
					      ex.what());
				}
			}

			// check audio device availability
			// Note: this only checks whether these libraries can be loaded.

			SPLog("Checking YSR availability");
			if (!audio::YsrDevice::TryLoadYsr()) {
				incapableConfigs.insert(
				  std::make_pair("s_audioDriver", [](std::string value) -> std::string {
					  if (EqualsIgnoringCase(value, "ysr")) {
						  return "YSR library couldn't be loaded.";
					  } else {
						  return std::string();
					  }
				  }));
			}

			SPLog("Checking OpenAL availability");
			if (!audio::ALDevice::TryLoad()) {
				incapableConfigs.insert(
				  std::make_pair("s_audioDriver", [](std::string value) -> std::string {
					  if (EqualsIgnoringCase(value, "openal")) {
						  return "OpenAL library couldn't be loaded.";
					  } else {
						  return std::string();
					  }
				  }));
			}

			// check openAL drivers
			SPLog("Checking OpenAL available drivers");
			openalDevices = audio::ALDevice::DeviceList();
			for (const auto &d: openalDevices) {
				SPLog("%s", d.c_str());
			}

			// check GL capabilities

			SPLog("Performing ecapability query");

			int idDisplay = 0;

			int numDisplayMode = SDL_GetNumDisplayModes(idDisplay);
			SDL_DisplayMode mode;
			modes.clear();
			if (numDisplayMode > 0) {
				std::set<std::pair<int, int>> foundModes;
				for (int i = 0; i < numDisplayMode; i++) {
					SDL_GetDisplayMode(idDisplay, i, &mode);
					if (mode.w < 800 || mode.h < 600)
						continue;
					if (foundModes.find(std::make_pair(mode.w, mode.h)) != foundModes.end())
						continue;

					foundModes.insert(std::make_pair(mode.w, mode.h));
					modes.push_back(spades::IntVector3::Make(mode.w, mode.h, 0));
					SPLog("Video Mode Found: %dx%d", mode.w, mode.h);
				}
			} else {
				SPLog("Failed to get video mode list. Presetting default list");
				modes.push_back(spades::IntVector3::Make(800, 600, 0));
				modes.push_back(spades::IntVector3::Make(1024, 768, 0));
				modes.push_back(spades::IntVector3::Make(1280, 720, 0));
				modes.push_back(spades::IntVector3::Make(1920, 1080, 0));
			}

			bool capable = true;
			SDL_Window *window = SDL_CreateWindow("OpenSpades: Please wait...", 1, 1, 1, 1,
			                                      SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
			if (window == nullptr) {
				SPLog("Failed to create SDL window: %s", SDL_GetError());
			}
			SDL_GLContext context = window ? SDL_GL_CreateContext(window) : nullptr;
			if (window != nullptr && context == nullptr) {
				SPLog("Failed to create OpenGL context: %s", SDL_GetError());
			}

#ifdef __APPLE__
#elif __unix
			SDL_Surface *icon = nullptr;
			SDL_RWops *icon_rw = nullptr;
			icon_rw = SDL_RWFromConstMem(g_appIconData, GetAppIconDataSize());
			if (icon_rw != nullptr) {
				icon = IMG_LoadPNG_RW(icon_rw);
				SDL_FreeRW(icon_rw);
			}
			if (icon == nullptr) {
				std::string msg = SDL_GetError();
				SPLog("Failed to load icon: %s", msg.c_str());
			} else {
				SDL_SetWindowIcon(window, icon);
				SDL_FreeSurface(icon);
			}
#endif

			if (!context) {
				// OpenGL initialization failed!

				std::string err = SDL_GetError();
				SPLog("SDL_SetVideoMode failed: %s", err.c_str());

				AddReport("OpenGL-capable graphics accelerator is unavailable.",
				          MakeVector4(1.f, 0.5f, 0.5f, 1.f));

				AddReport();

				AddReport("OpenGL/SDL couldn't be initialized.");
				AddReport("Falling back to the software renderer.");

				AddReport();
				AddReport("Message from SDL:", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				AddReport(err, MakeVector4(1.f, 1.f, 1.f, 0.7f));

				capable = false;
				shaderHighCapable = false;
				postFilterHighCapable = false;
				particleHighCapable = false;
			} else {

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

				AddReport("OpenGL-capable graphics accelerator is available.");

				if ((str = (const char *)glGetString(GL_VENDOR)) != NULL) {
					SPLog("Vendor: %s", str);
					AddReport(std::string("Vendor: ") + str, MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}
				if ((str = (const char *)glGetString(GL_RENDERER)) != NULL) {
					SPLog("Name: %s", str);
					AddReport(std::string("Name: ") + str, MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}
				if ((str = (const char *)glGetString(GL_VERSION)) != NULL) {
					AddReport(std::string("Version: ") + str, MakeVector4(1.f, 1.f, 1.f, 0.7f));
					SPLog("Version: %s", str);
				}
				if ((str = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION)) != NULL) {
					AddReport(std::string("GLSL Version: ") + str,
					          MakeVector4(1.f, 1.f, 1.f, 0.7f));
					SPLog("Shading Language Version: %s", str);
				}

				AddReport();

				maxTextureSize = 0;
				glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
				if (maxTextureSize > 0) {
					SPLog("Max Texture Size: %d", (int)maxTextureSize);
				}
				max3DTextureSize = 0;
				glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTextureSize);
				if (max3DTextureSize > 0) {
					SPLog("Max 3D Texture Size: %d", (int)max3DTextureSize);
				}

				maxCombinedTextureUnits = 0;
				glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);
				if (maxCombinedTextureUnits > 0) {
					SPLog("Max Combined Texture Image Units: %d", (int)maxCombinedTextureUnits);
				}

				maxVertexTextureUnits = 0;
				glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureUnits);
				if (maxVertexTextureUnits > 0) {
					SPLog("Max Vertex Texture Image Units: %d", (int)maxVertexTextureUnits);
				}

				maxVaryingComponents = 0;
				glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &maxVaryingComponents);
				if (maxVaryingComponents > 0) {
					SPLog("Max Varying Components: %d", (int)maxVaryingComponents);
				}

				str = (const char *)glGetString(GL_EXTENSIONS);
				std::string extensions;
				if (str)
					extensions = str;
				const char *const requiredExtensions[] = {"GL_ARB_multitexture",
				                                          "GL_ARB_shader_objects",
				                                          "GL_ARB_shading_language_100",
				                                          "GL_ARB_texture_non_power_of_two",
				                                          "GL_ARB_vertex_buffer_object",
				                                          "GL_EXT_framebuffer_object",
				                                          NULL};

				if (str) {
					SPLog("--- Extensions ---");
					std::vector<std::string> strs = spades::Split(str, " ");
					for (size_t i = 0; i < strs.size(); i++) {
						SPLog("%s", strs[i].c_str());
					}
				}
				SPLog("------------------");

				for (size_t i = 0; requiredExtensions[i]; i++) {
					const char *ex = requiredExtensions[i];
					if (extensions.find(ex) == std::string::npos) {
						// extension not found
						AddReport(std::string(ex) + " is NOT SUPPORTED!",
						          MakeVector4(1.f, 0.5f, 0.5f, 1.f));
						capable = false;
					}
				}

				// non-requred extensions
				if (extensions.find("GL_ARB_framebuffer_sRGB") == std::string::npos) {
					if (r_srgb) {
						r_srgb = 0;
						SPLog("Disabling r_srgb: no GL_ARB_framebuffer_sRGB");
					}

					incapableConfigs.insert(
					  std::make_pair("r_srgb", [](std::string value) -> std::string {
						  if (std::stoi(value) != 0) {
							  return "SRGB framebuffer is disabled because your video card doesn't "
							         "support GL_ARB_framebuffer_sRGB.";
						  } else {
							  return std::string();
						  }
					  }));

					AddReport("GL_ARB_framebuffer_sRGB is NOT SUPPORTED",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					AddReport("  r_srgb is disabled.", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}
				if (extensions.find("GL_EXT_framebuffer_blit") == std::string::npos) {
					if (r_blitFramebuffer) {
						r_blitFramebuffer = 0;
						SPLog("Disabling r_blitFramebuffer: no GL_EXT_framebuffer_blit");
					}
					if (r_multisamples) {
						r_multisamples = 0;
						SPLog("Disabling r_multisamples: no GL_EXT_framebuffer_blit");
					}
					if (r_temporalAA) {
						r_temporalAA = 0;
						SPLog("Disabling r_temporalAA: no GL_EXT_framebuffer_blit");
					}
					incapableConfigs.insert(
					  std::make_pair("r_blitFramebuffer", [](std::string value) -> std::string {
						  if (std::stoi(value) != 0) {
							  return "r_blitFramebuffer is disabled because your video card "
							         "doesn't support GL_EXT_framebuffer_blit.";
						  } else {
							  return std::string();
						  }
					  }));
					incapableConfigs.insert(
					  std::make_pair("r_temporalAA", [](std::string value) -> std::string {
						  if (std::stoi(value) != 0) {
							  return "r_temporalAA is disabled because your video card "
							         "doesn't support GL_EXT_framebuffer_blit.";
						  } else {
							  return std::string();
						  }
					  }));

					AddReport("GL_EXT_framebuffer_blit is NOT SUPPORTED",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					AddReport("  r_blitFramebuffer is disabled.", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}
				if (extensions.find("GL_EXT_texture_filter_anisotropic") == std::string::npos) {
					if ((float)r_maxAnisotropy > 1.1f) {
						r_maxAnisotropy = 1;
						SPLog("Setting r_maxAnisotropy to 1: no GL_EXT_texture_filter_anisotropic");
					}

					incapableConfigs.insert(
					  std::make_pair("r_maxAnisotropy", [](std::string value) -> std::string {
						  if (std::stof(value) > 1.001f) {
							  return "Anisotropic texture filtering is disabled because your video "
							         "card doesn't support GL_EXT_texture_filter_anisotropic.";
						  } else {
							  return std::string();
						  }
					  }));
					AddReport("GL_EXT_texture_filter_anisotropic is NOT SUPPORTED",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					AddReport("  r_maxAnisotropy is disabled.", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}

				if (extensions.find("GL_ARB_occlusion_query") == std::string::npos) {
					if (r_occlusionQuery) {
						r_occlusionQuery = 0;
						SPLog("Disabling r_occlusionQuery: no GL_ARB_occlusion_query");
					}
					incapableConfigs.insert(
					  std::make_pair("r_occlusionQuery", [](std::string value) -> std::string {
						  if (std::stoi(value)) {
							  return "Occlusion query is disabled because your video card doesn't "
							         "support GL_ARB_occlusion_query.";
						  } else {
							  return std::string();
						  }
					  }));
					AddReport("GL_ARB_occlusion_query is NOT SUPPORTED",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					AddReport("  r_occlusionQuery is disabled.", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}

				if (extensions.find("GL_NV_conditional_render") == std::string::npos) {
					if (r_occlusionQuery) {
						r_occlusionQuery = 0;
						SPLog("Disabling r_occlusionQuery: no GL_NV_conditional_render");
					}
					incapableConfigs.insert(
					  std::make_pair("r_occlusionQuery", [](std::string value) -> std::string {
						  if (std::stoi(value)) {
							  return "Occlusion query is disabled because your video card doesn't "
							         "support GL_NV_conditional_render.";
						  } else {
							  return std::string();
						  }
					  }));
					AddReport("GL_NV_conditional_render is NOT SUPPORTED",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					AddReport("  r_occlusionQuery is disabled.", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}

				if (extensions.find("GL_ARB_color_buffer_float") == std::string::npos) {
					if (r_hdr) {
						r_hdr = 0;
						SPLog("Disabling r_hdr: no GL_ARB_color_buffer_float");
					}
					incapableConfigs.insert(
					  std::make_pair("r_hdr", [](std::string value) -> std::string {
						  if (std::stoi(value)) {
							  return "HDR Rendering is disabled because your video card doesn't "
							         "support GL_ARB_color_buffer_float.";
						  } else {
							  return std::string();
						  }
					  }));
					AddReport("GL_ARB_color_buffer_float is NOT SUPPORTED",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					AddReport("  r_hdr is disabled.", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}

				if (extensions.find("GL_EXT_texture_array") == std::string::npos) {
					if ((int)r_water >= 2) {
						r_water = 1;
						SPLog("Disabling Water 2: no GL_EXT_texture_array");
					}
					shaderHighCapable = false;
					incapableConfigs.insert(
					  std::make_pair("r_water", [](std::string value) -> std::string {
						  if (std::stoi(value) >= 2) {
							  return "Water 2 is disabled because your video card doesn't "
							         "support GL_EXT_texture_array.";
						  } else {
							  return std::string();
						  }
					  }));
					AddReport("GL_EXT_texture_array is NOT SUPPORTED",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					AddReport("  Water 2 is disabled.", MakeVector4(1.f, 1.f, 1.f, 0.7f));
				}

				AddReport("Max Texture Size: " + std::to_string(maxTextureSize),
				          MakeVector4(1.f, 1.f, 1.f, 0.7f));
				if (maxTextureSize < 1024) {
					capable = false;
					AddReport("  TOO SMALL (1024 required)", MakeVector4(1.f, 0.5f, 0.5f, 1.f));
				}
				if ((int)r_shadowMapSize > maxTextureSize) {
					SPLog("Changed r_shadowMapSize from %d to %d: too small GL_MAX_TEXTURE_SIZE",
					      (int)r_shadowMapSize, maxTextureSize);

					r_shadowMapSize = maxTextureSize;
				}

				AddReport("Max 3D Texture Size: " + std::to_string(max3DTextureSize),
				          MakeVector4(1.f, 1.f, 1.f, 0.7f));
				if (max3DTextureSize < 512) {
					AddReport("  Global Illumination is disabled (512 required)",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					incapableConfigs.insert(
					  std::make_pair("r_radiosity", [](std::string value) -> std::string {
						  if (std::stoi(value)) {
							  return "Global illumination is disabled because your video card "
							         "doesn't support a 3D texture of at least 512x512x64.";
						  } else {
							  return std::string();
						  }
					  }));

					if (r_radiosity) {
						r_radiosity = 0;
						SPLog("Disabling r_radiosity: too small GL_MAX_3D_TEXTURE_SIZE");
					}
				}

				AddReport("Max Combined Texture Image Units: " +
				            std::to_string(maxCombinedTextureUnits),
				          MakeVector4(1.f, 1.f, 1.f, 0.7f));
				if (maxCombinedTextureUnits < 12) {
					AddReport("  Global Illumination is disabled (12 required)",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					incapableConfigs.insert(
					  std::make_pair("r_radiosity", [](std::string value) -> std::string {
						  if (std::stoi(value)) {
							  return "Global illumination is disabled because your video card "
							         "supports too few combined texture image units.";
						  } else {
							  return std::string();
						  }
					  }));

					if (r_radiosity) {
						r_radiosity = 0;
						SPLog(
						  "Disabling r_radiosity: too small GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS");
					}
				}
				if (maxCombinedTextureUnits < 15) {
					AddReport("  Water 2 is disabled (15 required)",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					shaderHighCapable = false;

					incapableConfigs.insert(
					  std::make_pair("r_water", [](std::string value) -> std::string {
						  if (std::stoi(value) >= 2) {
							  return "Water 2 is disabled because your video card supports too few "
							         "combined texture image units.";
						  } else {
							  return std::string();
						  }
					  }));

					if ((int)r_water >= 2) {
						r_water = 1;
						SPLog("Disabling Water 2: too small GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS");
					}
				}

				AddReport("Max Vertex Texture Image Units: " +
				            std::to_string(maxVertexTextureUnits),
				          MakeVector4(1.f, 1.f, 1.f, 0.7f));
				if (maxVertexTextureUnits < 3) {
					AddReport("  Water 2 is disabled (3 required)",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					shaderHighCapable = false;

					incapableConfigs.insert(
					  std::make_pair("r_water", [](std::string value) -> std::string {
						  if (std::stoi(value) >= 2) {
							  return "Water 2 is disabled because your video card supports too few "
							         "vertex texture image units.";
						  } else {
							  return std::string();
						  }
					  }));

					if ((int)r_water >= 2) {
						r_water = 1;
						SPLog("Disabling Water 2: too small GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS");
					}
				}

				AddReport("Max Varying Components: " + std::to_string(maxVaryingComponents),
				          MakeVector4(1.f, 1.f, 1.f, 0.7f));
				if (maxVaryingComponents < 37) {
					AddReport("  Shaded Particle is disabled (37 required)",
					          MakeVector4(1.f, 1.f, 0.5f, 1.f));
					particleHighCapable = false;

					incapableConfigs.insert(
					  std::make_pair("r_softParticles", [](std::string value) -> std::string {
						  if (std::stoi(value) >= 2) {
							  return "Shaded particle is disabled because your video card supports "
							         "too few varying fragment shader input components.";
						  } else {
							  return std::string();
						  }
					  }));

					if ((int)r_softParticles >= 2) {
						r_softParticles = 1;
						SPLog(
						  "Disabling shaded particle: too small GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS");
					}
				}

				AddReport();

				if (capable) {
					AddReport("Your video card supports all "
					          "required OpenGL extensions/features.",
					          MakeVector4(0.5f, 1.f, 0.5f, 1.f));
				} else {
					AddReport("Your video card/driver doesn't support "
					          "at least one of required OpenGL extensions/features."
					          " Falling back to the software renderer.",
					          MakeVector4(1.f, 0.5f, 0.5f, 1.f));
				}

				SDL_GL_DeleteContext(context);
				SDL_DestroyWindow(window);

				SPLog("SDL Capability Query Window finalized");
			}

			SPLog("OpenGL driver is OpenSpades capable: %s", capable ? "YES" : "NO");

			openGLCapable = capable;
			if (!openGLCapable) {
				if (spades::EqualsIgnoringCase(r_renderer, "gl")) {
					SPLog("Switched to software renderer");
					r_renderer = "sw";
				}

				incapableConfigs.insert(
				  std::make_pair("r_renderer", [](std::string value) -> std::string {
					  if (spades::EqualsIgnoringCase(value, "gl")) {
						  return "OpenGL renderer is disabled because "
						         "your video card/driver doesn't support "
						         "at least one of required OpenGL extensions/features.";
					  } else {
						  return std::string();
					  }
				  }));
			}
		}

		void StartupScreenHelper::FixConfigs() { ExamineSystem(); }

		std::string StartupScreenHelper::GetOperatingSystemType() {
#if defined(OS_PLATFORM_LINUX)
			return "Linux";
#elif defined(TARGET_OS_MAC)
			return "Mac";
#elif defined(OS_PLATFORM_WINDOWS)
			return "Windows";
#elif defined(__FreeBSD__)
			return "FreeBSD";
#elif defined(__DragonFly__)
			return "DragonFlyBSD";
#elif defined(__OpenBSD__)
			return "OpenBSD";
#elif defined(__sun)
			return "Solaris";
#elif defined(__HAIKU__)
			return "Haiku";
#else
			return std::string{};
#endif
		}

		PackageUpdateManager& StartupScreenHelper::GetPackageUpdateManager() {
			return PackageUpdateManager::GetInstance();
		}

		bool StartupScreenHelper::OpenUpdateInfoURL() {
			std::string url = GetPackageUpdateManager().GetLatestVersionInfoPageURL();
			if (url.find("http:") != 0 && url.find("https:") != 0) {
				return false;
			}
			return OpenURLInBrowser(url);
		}

		bool StartupScreenHelper::BrowseUserDirectory() {
			std::string path = g_userResourceDirectory;

			if (path.empty()) {
				SPLog("Cannot open the user resource directory: g_userResourceDirectory is empty.");
				return false;
			}

			return ShowDirectoryInShell(path);
		}

		void StartupScreenHelper::Start() {
			if (scr == nullptr) {
				return;
			}
			scr->Start();
		}

		int StartupScreenHelper::GetNumVideoModes() { return static_cast<int>(modes.size()); }

		int StartupScreenHelper::GetVideoModeWidth(int index) {
			if (index < 0 || index >= GetNumVideoModes())
				SPInvalidArgument("index");
			return modes[index].x;
		}
		int StartupScreenHelper::GetVideoModeHeight(int index) {
			if (index < 0 || index >= GetNumVideoModes())
				SPInvalidArgument("index");
			return modes[index].y;
		}

		int StartupScreenHelper::GetNumReportLines() {
			return static_cast<int>(reportLines.size());
		}

		std::string StartupScreenHelper::GetReportLineText(int index) {
			if (index < 0 || index >= GetNumReportLines())
				SPInvalidArgument("index");
			return reportLines[index].text;
		}
		Vector4 StartupScreenHelper::GetReportLineColor(int index) {
			if (index < 0 || index >= GetNumReportLines())
				SPInvalidArgument("index");
			return reportLines[index].color;
		}

		void StartupScreenHelper::AddReport(const std::string &text, Vector4 color) {
			ReportLine l = {text, color};
			reportLines.push_back(l);
			report += text;
			report += '\n';
		}

		int StartupScreenHelper::GetNumAudioOpenALDevices() { return static_cast<int>(openalDevices.size()); }
		std::string StartupScreenHelper::GetAudioOpenALDevice(int index) {
			if (index < 0 || index >= GetNumAudioOpenALDevices())
				SPInvalidArgument("index");
			return openalDevices[index];
		}

		int StartupScreenHelper::GetNumLocales() { return static_cast<int>(locales.size()); }
		std::string StartupScreenHelper::GetLocale(int index) {
			if (index < 0 || index >= GetNumLocales())
				SPInvalidArgument("index");
			return locales[index].name;
		}
		std::string StartupScreenHelper::GetLocaleDescriptionNative(int index) {
			if (index < 0 || index >= GetNumLocales())
				SPInvalidArgument("index");
			return locales[index].descriptionNative;
		}
		std::string StartupScreenHelper::GetLocaleDescriptionEnglish(int index) {
			if (index < 0 || index >= GetNumLocales())
				SPInvalidArgument("index");
			return locales[index].descriptionEnglish;
		}

		std::string StartupScreenHelper::CheckConfigCapability(const std::string &cfg,
		                                                       const std::string &value) {
			auto range = incapableConfigs.equal_range(cfg);
			std::string ret;
			bool hasMulti = false;
			for (auto it = range.first; it != range.second; it++) {
				auto &f = it->second;
				auto err = f(value);
				if (err.size() > 0) {
					if (ret.size() == 0) {
						ret = err;
					} else {
						if (!hasMulti) {
							ret = "- " + ret;
							hasMulti = true;
						}
						ret += "\n- " + err;
					}
				}
			}
			return ret;
		}
	}
}
