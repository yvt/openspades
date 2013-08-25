//
//  SDLGLDevice.cpp
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "SDLGLDevice.h"
#include "../Imports/OpenGL.h"
#include "../Imports/SDL.h"
#include "../Core/Exception.h"
#include "../Core/Debug.h"
#include "../Core/Settings.h"

using namespace spades::draw;

#ifndef __APPLE__
#define GLEW 1
#endif

SPADES_SETTING(r_ignoreGLErrors, "1");

static uint32_t vertCount = 0;
static uint32_t drawOps = 0;
static Uint32 lastFrame = 0;

namespace spades {
	namespace gui {
		
#define CheckError() do{ \
		GLenum err; \
		if(!r_ignoreGLErrors){ \
			 err = glGetError(); \
			if(err != GL_NO_ERROR) \
				ReportError(err, __LINE__, __PRETTY_FUNCTION__); \
		} \
	}while(0)
		
#define CheckExistence(func) do { \
		if(!func) { \
			ReportMissingFunc( #func ); \
		}\
	} while(0)
		
		static void ReportError(GLenum err, int line, const char *func){
			SPRaise("GL error 0x%08x in %s at %s:%d",
					(int)err,
					func, __FILE__, line);
		}
		static void ReportMissingFunc(const char *func){
			SPRaise("GL function %s missing",
					func);
		}
		
		SDLGLDevice::SDLGLDevice(SDL_Surface *s):
		surface(s) {
#ifndef __APPLE__
			GLenum err = glewInit();
			if (GLEW_OK != err){
				SPRaise("GLEW error: %s",  glewGetErrorString(err));
			}
#endif
			SPLog("SDLGLDevice created");
			
			
			SPLog("--- OpenGL Renderer Info ---");
			const char *ret;
			if((ret = (const char*)glGetString(GL_VENDOR)) != NULL) {
				SPLog("Vendor: %s", ret);
			}
			if((ret = (const char*)glGetString(GL_RENDERER)) != NULL) {
				SPLog("Name: %s", ret);
			}
			if((ret = (const char*)glGetString(GL_VERSION)) != NULL) {
				SPLog("Version: %s", ret);
			}
			if((ret = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)) != NULL) {
				SPLog("Shading Language Version: %s", ret);
			}
			SPLog("--- Extensions ---");
			if(glGetStringi) {
				GLint cnt = 0;
				glGetIntegerv(GL_NUM_EXTENSIONS, &cnt);
				if(cnt <= 0) goto retrvFail;
				for(GLint i = 0; i < cnt; i++) {
					ret = (const char*)glGetStringi(GL_EXTENSIONS, i);
					SPLog("%s", ret);
				}
			}else{
			retrvFail:
				if((ret = (const char*)glGetString(GL_EXTENSIONS)) != NULL) {
					std::vector<std::string> strs = Split(ret, " ");
					for(size_t i = 0; i < strs.size(); i++)
					SPLog("%s", strs[i].c_str());
				}else{
					SPLog("no information");
				}
				
			}
			SPLog("------------------");
			
			CheckExistence(glFrontFace);
			glFrontFace(GL_CW);
		}
		
		void SDLGLDevice::DepthRange(Float near, Float far){
			CheckExistence(glDepthRange);
			glDepthRange(near, far);
			CheckError();
		}
		void SDLGLDevice::Viewport(Integer x, Integer y,
								   Sizei width, Sizei height) {
			CheckExistence(glViewport);
			glViewport(x, y, width, height);
			CheckError();
		}
		
		void SDLGLDevice::ClearDepth(float v) {
			CheckExistence(glClearDepth);
			glClearDepth(v);
			CheckError();
		}
		void SDLGLDevice::ClearColor(float r, float g, float b, float a){
			CheckExistence(glClearColor);
			glClearColor(r, g, b, a);
			CheckError();
		}
		void SDLGLDevice::Clear(Enum bits) {
			GLbitfield v = 0;
			if(bits & ColorBufferBit)
				v |= GL_COLOR_BUFFER_BIT;
			if(bits & DepthBufferBit)
				v |= GL_DEPTH_BUFFER_BIT;
			if(bits & StencilBufferBit)
				v |= GL_STENCIL_BUFFER_BIT;
			CheckExistence(glClear);
			glClear(v);
			CheckError();
		}
		
		void SDLGLDevice::Swap() {
			//glFinish();
			SDL_GL_SwapBuffers();
#if 0
			Uint32 t = SDL_GetTicks();
			if(lastFrame == 0) t = lastFrame - 30;
			double dur = (double)(t - lastFrame) / 1000.;
			lastFrame = t;
			
			printf("FPS:%.02f, Vertices: %d (%.02f/sec), Drawcalls: %d (%.02f/sec)\n", 1./dur,
				   vertCount, vertCount / dur,
				   drawOps, drawOps / dur);
			
			//printf("%.02f,%.02f,%.02f\n", 1./dur, vertCount / dur, drawOps / dur);
#endif
			vertCount = 0;
			drawOps = 0;
		}
		
		void SDLGLDevice::Finish() {
			CheckExistence(glFinish);
			glFinish();
			CheckError();
		}
		void SDLGLDevice::Flush(){
			CheckExistence(glFlush);
			glFlush();
			CheckError();
		}
		
		void SDLGLDevice::DepthMask(bool b){
			CheckExistence(glDepthMask);
			glDepthMask(b?GL_TRUE:GL_FALSE);
			CheckError();
		}
		
		void SDLGLDevice::ColorMask(bool r, bool g, bool b, bool a){
			CheckExistence(glColorMask);
			glColorMask(r?GL_TRUE:GL_FALSE,
						g?GL_TRUE:GL_FALSE,
						b?GL_TRUE:GL_FALSE,
						a?GL_TRUE:GL_FALSE);
			CheckError();
		}
		
		void SDLGLDevice::Enable(spades::draw::IGLDevice::Enum state, bool b){
			SPADES_MARK_FUNCTION();
			GLenum type;
			switch(state){
				case DepthTest: type = GL_DEPTH_TEST; break;
				case CullFace: type = GL_CULL_FACE; break;
				case Blend: type = GL_BLEND; break;
				case Texture2D: type = GL_TEXTURE_2D; break;
				case Multisample: type = GL_MULTISAMPLE; break;
				case FramebufferSRGB: type = GL_FRAMEBUFFER_SRGB; break;
				default: SPInvalidEnum("state", state);
			}
			if(b)
				glEnable(type);
			else
				glDisable(type);
			CheckError();
		}
		
		IGLDevice::Integer SDLGLDevice::GetInteger(Enum type){
			SPADES_MARK_FUNCTION();
			GLint v;
			switch(type){
				case draw::IGLDevice::FramebufferBinding:
					glGetIntegerv(GL_FRAMEBUFFER_BINDING, &v);
					break;
				default:
					SPInvalidEnum("type", type);
			}
			CheckError();
			return v;
		}
		
		const char *SDLGLDevice::GetString(spades::draw::IGLDevice::Enum type) {
			SPADES_MARK_FUNCTION();
			switch(type){
				case Vendor:
					return (const char *)glGetString(GL_VENDOR);
				case Renderer:
					return (const char *)glGetString(GL_RENDERER);
				case Version:
					return (const char *)glGetString(GL_VERSION);
				case ShadingLanguageVersion:
					return (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
				default: SPInvalidEnum("type", type);
			}
		}
		const char *SDLGLDevice::GetIndexedString(spades::draw::IGLDevice::Enum type, UInteger index) {
			SPADES_MARK_FUNCTION();
			switch(type){
				case draw::IGLDevice::Extensions:
					return (const char *)glGetStringi(GL_EXTENSIONS, index);
				default: SPInvalidEnum("type", type);
			}
		}
		
		GLenum SDLGLDevice::parseBlendEquation(spades::draw::IGLDevice::Enum v){
			SPADES_MARK_FUNCTION();
			switch(v){
				case Add: return GL_FUNC_ADD;
				case Subtract: return GL_FUNC_SUBTRACT;
				case ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
				case MinOp: return GL_MIN;
				case MaxOp: return GL_MAX;
				default: SPInvalidEnum("v", v);
			}
		}
		
		GLenum SDLGLDevice::parseBlendFunction(spades::draw::IGLDevice::Enum v){
			SPADES_MARK_FUNCTION();
			switch(v){
				case Zero: return GL_ZERO;
				case One: return GL_ONE;
				case SrcColor: return GL_SRC_COLOR;
				case DestColor: return GL_DST_COLOR;
				case OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
				case OneMinusDestColor: return GL_ONE_MINUS_DST_COLOR;
				case SrcAlpha: return GL_SRC_ALPHA;
				case DestAlpha: return GL_DST_ALPHA;
				case OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
				case OneMinusDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
				case ConstantColor: return GL_CONSTANT_COLOR;
				case ConstantAlpha: return GL_CONSTANT_ALPHA;
				case OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
				case OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
				default: SPInvalidEnum("v", v);
			}
		}
		
		void SDLGLDevice::BlendEquation(spades::draw::IGLDevice::Enum mode) {
			CheckExistence(glBlendEquation);
			glBlendEquation(parseBlendEquation(mode));
			CheckError();
		}
		
		void SDLGLDevice::BlendEquation(spades::draw::IGLDevice::Enum rgb,
										spades::draw::IGLDevice::Enum alpha) {
			CheckExistence(glBlendEquationSeparate);
			glBlendEquationSeparate(parseBlendEquation(rgb),
									parseBlendEquation(alpha));
			CheckError();
		}
		void SDLGLDevice::BlendFunc(Enum src,
									Enum dest) {
			CheckExistence(glBlendFunc);
			glBlendFunc(parseBlendFunction(src),
						parseBlendFunction(dest));
			CheckError();
		}
		void SDLGLDevice::BlendFunc(Enum srcRgb,
									Enum destRgb,
									Enum srcAlpha,
									Enum destAlpha) {
			CheckExistence(glBlendFuncSeparate);
			glBlendFuncSeparate(parseBlendFunction(srcRgb),
								parseBlendFunction(destRgb),
								parseBlendFunction(srcAlpha),
								parseBlendFunction(destAlpha));
			CheckError();
		}
		void SDLGLDevice::BlendColor(Float r, Float g, Float b, Float a){
			CheckExistence(glBlendColor);
			glBlendColor(r, g, b, a);
			CheckError();
		}
		void SDLGLDevice::LineWidth(Float w) {
			CheckExistence(glLineWidth);
			glLineWidth(w);
			CheckError();
		}
		void SDLGLDevice::DepthFunc(Enum func) {
			SPADES_MARK_FUNCTION();
			CheckExistence(glDepthFunc);
			switch(func){
				case Never: glDepthFunc(GL_NEVER); break;
				case Always: glDepthFunc(GL_ALWAYS); break;
				case Less: glDepthFunc(GL_LESS); break;
				case LessOrEqual: glDepthFunc(GL_LEQUAL); break;
				case Equal: glDepthFunc(GL_EQUAL); break;
				case Greater: glDepthFunc(GL_GREATER); break;
				case GreaterOrEqual: glDepthFunc(GL_GEQUAL); break;
				case NotEqual: glDepthFunc(GL_NOTEQUAL); break;
				default: SPInvalidEnum("func", func);
			}
			CheckError();
		}
		
		IGLDevice::UInteger SDLGLDevice::GenBuffer() {
			SPADES_MARK_FUNCTION_DEBUG();
			GLuint i;
			CheckExistence(glGenBuffers);
			glGenBuffers(1, &i);
			CheckError();
			return i;
		}
		
		void SDLGLDevice::DeleteBuffer(UInteger i) {
			SPADES_MARK_FUNCTION_DEBUG();
			GLuint v = (GLuint)i;
			CheckExistence(glDeleteBuffers);
			glDeleteBuffers(1, &v);
			CheckError();
		}
		
		void *SDLGLDevice::MapBuffer(Enum target,
								   Enum access) {
			SPADES_MARK_FUNCTION_DEBUG();
			GLenum acc;
			switch(access){
				case draw::IGLDevice::ReadOnly:
					acc = GL_READ_ONLY;
					break;
				case draw::IGLDevice::WriteOnly:
					acc = GL_WRITE_ONLY;
					break;
				case draw::IGLDevice::ReadWrite:
					acc = GL_READ_WRITE;
					break;
				default:
					SPInvalidEnum("access", access);
			}
			CheckExistence(glMapBuffer);
			void *ret = glMapBuffer(parseBufferTarget(target),
									acc);
			CheckError();
			return ret;
		}
		
		void SDLGLDevice::UnmapBuffer(Enum target) {
			CheckExistence(glUnmapBuffer);
			glUnmapBuffer(parseBufferTarget(target));
			CheckError();
		}
		
		GLenum SDLGLDevice::parseBufferTarget(spades::draw::IGLDevice::Enum v){
			SPADES_MARK_FUNCTION_DEBUG();
			switch(v){
				case ArrayBuffer: return GL_ARRAY_BUFFER;
				case ElementArrayBuffer: return GL_ELEMENT_ARRAY_BUFFER;
				case PixelPackBuffer: return GL_PIXEL_PACK_BUFFER;
				case PixelUnpackBuffer: return GL_PIXEL_UNPACK_BUFFER;
				default: SPInvalidEnum("v", v);
			}
		}
		
		void SDLGLDevice::BindBuffer(Enum target, UInteger i){
			CheckExistence(glBindBuffer);
			glBindBuffer(parseBufferTarget(target), (GLuint)i);
			CheckError();
		}
		
		void SDLGLDevice::BufferData(Enum target, Sizei size,
									 const void *data,
									 Enum usage) {
			SPADES_MARK_FUNCTION();
			GLenum usageVal;
			switch(usage){
				case StaticDraw: usageVal = GL_STATIC_DRAW; break;
				case StreamDraw: usageVal = GL_STREAM_DRAW; break;
				case DynamicDraw: usageVal = GL_DYNAMIC_DRAW; break;
				default: SPInvalidEnum("usage", usage);
			}
			CheckExistence(glBufferData);
			glBufferData(parseBufferTarget(target), (GLsizeiptr)size,
						 data, usageVal);
			CheckError();
		}
		void SDLGLDevice::BufferSubData(Enum target, Sizei offset,
										Sizei size, const void *data) {
			CheckExistence(glBufferSubData);
			glBufferSubData(parseBufferTarget(target), offset,
							size, data);
			CheckError();
		}
		
		IGLDevice::UInteger SDLGLDevice::GenTexture() {
			GLuint i;
			CheckExistence(glGenTextures);
			glGenTextures(1, &i);
			return i;
		}
		
		void SDLGLDevice::DeleteTexture(UInteger i) {
			GLuint v = (GLuint)i;
			CheckExistence(glDeleteTextures);
			glDeleteTextures(1, &v);
			CheckError();
		}
		
		GLenum SDLGLDevice::parseTextureTarget(Enum v) {
			SPADES_MARK_FUNCTION_DEBUG();
			switch(v){
				case Texture2D: return GL_TEXTURE_2D;
				case Texture3D: return GL_TEXTURE_3D;
				default: SPInvalidEnum("v", v);
			}
		}
		
		void SDLGLDevice::ActiveTexture(UInteger stage) {
			CheckExistence(glActiveTexture);
			glActiveTexture(GL_TEXTURE0 + stage);
			CheckError();
		}
		
		void SDLGLDevice::BindTexture(Enum target, UInteger tex){
			CheckExistence(glBindTexture);
			glBindTexture(parseTextureTarget(target), tex);
			CheckError();
		}
		
		GLenum SDLGLDevice::parseTextureInternalFormat(Enum v) {
			SPADES_MARK_FUNCTION_DEBUG();
			switch(v){
				case Red: return GL_RED;
				case RG: return GL_RG;
				case RGB: return GL_RGB;
				case RGBA: return GL_RGBA;
				case DepthComponent: return GL_DEPTH_COMPONENT;
				case DepthComponent24: return GL_DEPTH_COMPONENT24;
				case StencilIndex: return GL_STENCIL_INDEX;
					
				case RGB10A2: return GL_RGB10_A2;
				case RGB16F: return GL_RGB16F;
				case RGBA16F: return GL_RGBA16F;
				case R16F: return GL_R16F;
				case RGB5: return GL_RGB5;
				case RGB5A1: return GL_RGB5_A1;
				default: SPInvalidEnum("v", v);
			}
		}
		
		GLenum SDLGLDevice::parseTextureFormat(Enum v) {
			SPADES_MARK_FUNCTION_DEBUG();
			switch(v){
				case Red: return GL_RED;
				case RG: return GL_RG;
				case RGB: return GL_RGB;
				case RGBA: return GL_RGBA;
				case BGRA: return GL_BGRA;
				case DepthComponent: return GL_DEPTH_COMPONENT;
				case StencilIndex: return GL_STENCIL_INDEX;
				default: SPInvalidEnum("v", v);
			}
		}
		
		GLenum SDLGLDevice::parseType(Enum v) {
			SPADES_MARK_FUNCTION_DEBUG();
			switch(v){
				case Int: return GL_INT;
				case UnsignedInt: return GL_UNSIGNED_INT;
				case Short: return GL_SHORT;
				case UnsignedShort: return GL_UNSIGNED_SHORT;
				case Byte: return GL_BYTE;
				case UnsignedByte: return GL_UNSIGNED_BYTE;
				case FloatType: return GL_FLOAT;
				case UnsignedShort5551: return GL_UNSIGNED_SHORT_5_5_5_1;
				case UnsignedShort1555Rev: return GL_UNSIGNED_SHORT_1_5_5_5_REV;
				case UnsignedInt2101010Rev: return GL_UNSIGNED_INT_2_10_10_10_REV;
				default: SPInvalidEnum("v", v);
			}
		}
		
		void SDLGLDevice::TexImage2D(Enum target, Integer level,
									 Enum intFmt,
									 Sizei width, Sizei height,
									 Integer border,
									 Enum format, Enum type,
									 const void *data) {
			CheckExistence(glTexImage2D);
			glTexImage2D(parseTextureTarget(target),
						 level, parseTextureInternalFormat(intFmt),
						 width, height, border,
						 parseTextureFormat(format),
						 parseType(type), data);
			CheckError();
		}
		
		void SDLGLDevice::TexImage3D(Enum target, Integer level,
									 Enum intFmt,
									 Sizei width, Sizei height,
									 Sizei depth,
									 Integer border,
									 Enum format, Enum type,
									 const void *data) {
			CheckExistence(glTexImage3D);
			glTexImage3D(parseTextureTarget(target),
						 level, parseTextureInternalFormat(intFmt),
						 width, height, depth, border,
						 parseTextureFormat(format),
						 parseType(type), data);
			CheckError();
		}
		
		void SDLGLDevice::TexSubImage2D(Enum target,
										Integer level,
										Integer x, Integer y,
										Sizei width, Sizei height,
										Enum format, Enum type,
										const void *data) {
			CheckExistence(glTexSubImage2D);
			glTexSubImage2D(parseTextureTarget(target), level,
							x, y, width, height,
							parseTextureFormat(format),
							parseType(type), data);
			CheckError();
		}
		
		void SDLGLDevice::TexSubImage3D(Enum target,
										Integer level,
										Integer x, Integer y,
										Integer z,
										Sizei width, Sizei height,
										Sizei depth,
										Enum format, Enum type,
										const void *data) {
			CheckExistence(glTexSubImage3D);
			glTexSubImage3D(parseTextureTarget(target), level,
							x, y, z, width, height, depth,
							parseTextureFormat(format),
							parseType(type), data);
			CheckError();
		}
		
		void SDLGLDevice::CopyTexSubImage2D(Enum target,
											Integer level,
											Integer destinationX,
											Integer destinationY,
											Integer srcX,
											Integer srcY,
											Sizei width,
											Sizei height) {
			CheckExistence(glCopyTexSubImage2D);
			glCopyTexSubImage2D(parseTextureTarget(target), level,
								destinationX, destinationY,
								srcX, srcY, width, height);
			CheckError();
		}
		
		void SDLGLDevice::TexParamater(Enum target,Enum param, Enum val) {
			SPADES_MARK_FUNCTION();
			GLenum targ = parseTextureTarget(target);
			CheckExistence(glTexParameteri);
			switch(param){
				case TextureMinFilter:
					switch(val){
						case Nearest: glTexParameteri(targ, GL_TEXTURE_MIN_FILTER, GL_NEAREST); break;
						case Linear: glTexParameteri(targ, GL_TEXTURE_MIN_FILTER, GL_LINEAR); break;
						case NearestMipmapLinear: glTexParameteri(targ, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR); break;
						case LinearMipmapLinear: glTexParameteri(targ, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); break;
						case NearestMipmapNearest: glTexParameteri(targ, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); break;
						case LinearMipmapNearest: glTexParameteri(targ, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); break;
						default:
							SPInvalidEnum("val", val);
					}
					break;
				case TextureMagFilter:
					switch(val){
						case Nearest: glTexParameteri(targ, GL_TEXTURE_MAG_FILTER, GL_NEAREST); break;
						case Linear: glTexParameteri(targ, GL_TEXTURE_MAG_FILTER, GL_LINEAR); break;
						default:
							SPInvalidEnum("val", val);
					}
					break;
				case TextureWrapS:
					switch(val){
						case ClampToEdge: glTexParameteri(targ, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); break;
						case Repeat: glTexParameteri(targ, GL_TEXTURE_WRAP_S, GL_REPEAT); break;
						default:
							SPInvalidEnum("val", val);
					}
					break;
				case TextureWrapT:
					switch(val){
						case ClampToEdge: glTexParameteri(targ, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); break;
						case Repeat: glTexParameteri(targ, GL_TEXTURE_WRAP_T, GL_REPEAT); break;
						default:
							SPInvalidEnum("val", val);
					}
					break;
				case TextureWrapR:
					switch(val){
						case ClampToEdge: glTexParameteri(targ, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); break;
						case Repeat: glTexParameteri(targ, GL_TEXTURE_WRAP_R, GL_REPEAT); break;
						default:
							SPInvalidEnum("val", val);
					}
					break;
				case TextureCompareMode:
					switch(val){
						case draw::IGLDevice::CompareRefToTexture: glTexParameteri(targ, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE); break;
						case draw::IGLDevice::None: glTexParameteri(targ, GL_TEXTURE_COMPARE_MODE, GL_NONE); break;
						default:
							SPInvalidEnum("val", val);
					}
					break;
				case TextureCompareFunc:
					switch(val){
						case IGLDevice::LessOrEqual: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL); break;
						case IGLDevice::GreaterOrEqual: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL); break;
						case IGLDevice::Less: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_LESS); break;
						case IGLDevice::Greater: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_GREATER); break;
						case IGLDevice::Equal: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_EQUAL); break;
						case IGLDevice::NotEqual: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_NOTEQUAL); break;
						case IGLDevice::Always: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS); break;
						case IGLDevice::Never: glTexParameteri(targ, GL_TEXTURE_COMPARE_FUNC, GL_NEVER); break;
						default:
							SPInvalidEnum("val", val);
					}
					break;
				default:
					SPInvalidEnum("param", param);
			}
			
			CheckError();
		}
		
		void SDLGLDevice::GenerateMipmap(spades::draw::IGLDevice::Enum target){
			CheckExistence(glGenerateMipmap);
			glGenerateMipmap(parseTextureTarget(target));
			CheckError();
		}
		
		void SDLGLDevice::VertexAttrib(UInteger index, Float x) {
			CheckExistence(glVertexAttrib1f);
			glVertexAttrib1f(index, x);
			CheckError();
		}
		
		void SDLGLDevice::VertexAttrib(UInteger index, Float x, Float y) {
			CheckExistence(glVertexAttrib2f);
			glVertexAttrib2f(index, x, y);
			CheckError();
		}
		
		void SDLGLDevice::VertexAttrib(UInteger index, Float x, Float y, Float z) {
			CheckExistence(glVertexAttrib3f);
			glVertexAttrib3f(index, x, y, z);
			CheckError();
		}
		
		void SDLGLDevice::VertexAttrib(UInteger index, Float x, Float y, Float z, Float w) {
			CheckExistence(glVertexAttrib4f);
			glVertexAttrib4f(index, x, y, z, w);
			CheckError();
		}
		
		void SDLGLDevice::VertexAttribPointer(UInteger index,
											  Integer size,
											  Enum type, bool normalized,
											  Sizei stride,
											  const void *data) {
			CheckExistence(glVertexAttribPointer);
			glVertexAttribPointer(index, size, parseType(type),
								  normalized, stride, data);
			CheckError();
		}
		
		void SDLGLDevice::VertexAttribIPointer(UInteger index,
											  Integer size,
											  Enum type,
											  Sizei stride,
											  const void *data) {
			CheckExistence(glVertexAttribIPointer);
			glVertexAttribIPointer(index, size, parseType(type),
								   stride, data);
			CheckError();
		}
		
		void SDLGLDevice::EnableVertexAttribArray(UInteger index, bool b){
			CheckExistence(glEnableVertexAttribArray);
			CheckExistence(glDisableVertexAttribArray);
			if(b)
				glEnableVertexAttribArray(index);
			else
				glDisableVertexAttribArray(index);
			CheckError();
		}
		
		void SDLGLDevice::VertexAttribDivisor(UInteger index, UInteger divisor) {
			CheckExistence(glVertexAttribDivisorARB);
			glVertexAttribDivisorARB(index, divisor);
			CheckError();
		}
		
		void SDLGLDevice::DrawArrays(Enum mode, Integer first, Sizei count) {
			SPADES_MARK_FUNCTION();
			GLenum md;
			switch(mode){
				case Points: md = GL_POINTS; break;
				case LineStrip: md = GL_LINE_STRIP; break;
				case LineLoop: md = GL_LINE_LOOP; break;
				case Lines: md = GL_LINES; break;
				case TriangleStrip: md = GL_TRIANGLE_STRIP; break;
				case TriangleFan: md = GL_TRIANGLE_FAN; break;
				case Triangles: md = GL_TRIANGLES; break;
				default: SPInvalidEnum("mode", mode);
			}
			vertCount += count;
			drawOps++;
			CheckExistence(glDrawArrays);
			glDrawArrays(md, first, count);
			CheckError();
		}
		
		void SDLGLDevice::DrawElements(Enum mode, Sizei count, Enum type, const void *indices) {
			SPADES_MARK_FUNCTION();
			GLenum md;
			switch(mode){
				case Points: md = GL_POINTS; break;
				case LineStrip: md = GL_LINE_STRIP; break;
				case LineLoop: md = GL_LINE_LOOP; break;
				case Lines: md = GL_LINES; break;
				case TriangleStrip: md = GL_TRIANGLE_STRIP; break;
				case TriangleFan: md = GL_TRIANGLE_FAN; break;
				case Triangles: md = GL_TRIANGLES; break;
				default: SPInvalidEnum("mode", mode);
			}
			vertCount += count;
			drawOps++;
			CheckExistence(glDrawElements);
			glDrawElements(md, count, parseType(type), indices);
			CheckError();
		}
		
		void SDLGLDevice::DrawArraysInstanced(Enum mode, Integer first, Sizei count,
											  Sizei instances) {
			SPADES_MARK_FUNCTION();
			GLenum md;
			switch(mode){
				case Points: md = GL_POINTS; break;
				case LineStrip: md = GL_LINE_STRIP; break;
				case LineLoop: md = GL_LINE_LOOP; break;
				case Lines: md = GL_LINES; break;
				case TriangleStrip: md = GL_TRIANGLE_STRIP; break;
				case TriangleFan: md = GL_TRIANGLE_FAN; break;
				case Triangles: md = GL_TRIANGLES; break;
				default: SPInvalidEnum("mode", mode);
			}
#if GLEW
			if(glDrawArraysInstanced)
				glDrawArraysInstanced(md, first, count, instances);
			else if(glDrawArraysInstancedARB)
				glDrawArraysInstancedARB(md, first, count, instances);
			else if(glDrawArraysInstancedEXT)
				glDrawArraysInstancedEXT(md, first, count, instances);
			else
				SPRaise("glDrawArraysInstanced not found");
#else
			glDrawArraysInstanced(md, first, count, instances);
#endif
			CheckError();
			vertCount += count * instances;
			drawOps++;
		}
		
		void SDLGLDevice::DrawElementsInstanced(Enum mode, Sizei count, Enum type, const void *indices,
												Sizei instances) {
			SPADES_MARK_FUNCTION();
			GLenum md;
			switch(mode){
				case Points: md = GL_POINTS; break;
				case LineStrip: md = GL_LINE_STRIP; break;
				case LineLoop: md = GL_LINE_LOOP; break;
				case Lines: md = GL_LINES; break;
				case TriangleStrip: md = GL_TRIANGLE_STRIP; break;
				case TriangleFan: md = GL_TRIANGLE_FAN; break;
				case Triangles: md = GL_TRIANGLES; break;
				default: SPInvalidEnum("mode", mode);
			}
#if GLEW
			if(glDrawElementsInstanced)
				glDrawElementsInstanced(md, count, parseType(type), indices, instances);
			else if(glDrawElementsInstancedARB)
				glDrawElementsInstancedARB(md, count, parseType(type), indices, instances);
			else if(glDrawElementsInstancedEXT)
				glDrawElementsInstancedEXT(md, count, parseType(type), indices, instances);
			else
				SPRaise("glDrawElementsInstanced not found");
#else
			glDrawElementsInstanced(md, count, parseType(type), indices, instances);
#endif
			CheckError();
			vertCount += count * instances;
			drawOps++;
		}
		
		IGLDevice::UInteger SDLGLDevice::CreateShader(Enum type) {
			SPADES_MARK_FUNCTION();
			CheckExistence(glCreateShader);
			switch(type){
				case draw::IGLDevice::FragmentShader:
					return glCreateShader(GL_FRAGMENT_SHADER);
				case draw::IGLDevice::VertexShader:
					return glCreateShader(GL_VERTEX_SHADER);
				default: SPInvalidEnum("type", type);
			}
		}
		
		void SDLGLDevice::ShaderSource(UInteger shader, Sizei count,
									   const char **string, const int *len) {
			CheckExistence(glShaderSource);
			glShaderSource(shader, count, (const GLchar **)string, len);
			CheckError();
		}
		
		void SDLGLDevice::CompileShader(UInteger i) {
			CheckExistence(glCompileShader);
			glCompileShader(i);
			CheckError();
		}
		
		void SDLGLDevice::DeleteShader(UInteger i){
			CheckExistence(glDeleteShader);
			glDeleteShader(i);
			CheckError();
		}
		
		IGLDevice::Integer SDLGLDevice::GetShaderInteger(UInteger shader,
														 Enum param) {
			SPADES_MARK_FUNCTION();
			GLint ret;
			CheckExistence(glGetShaderiv);
			switch(param){
				case ShaderType:
					glGetShaderiv(shader, GL_SHADER_TYPE, &ret);
					break;
				case DeleteStatus:
					glGetShaderiv(shader, GL_DELETE_STATUS, &ret);
					break;
				case CompileStatus:
					glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
					break;
				case InfoLogLength:
					glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &ret);
					break;
				case ShaderSourceLength:
					glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &ret);
					break;
				default:
					SPInvalidEnum("param", param);
			}
			CheckError();
			return ret;
		}
		
		void SDLGLDevice::GetShaderInfoLog(UInteger shader, Sizei bufferSize,
										   Sizei *length, char *outString) {
			CheckExistence(glGetShaderInfoLog);
			glGetShaderInfoLog(shader, bufferSize, (GLsizei *)length,
							   (GLchar *)outString);
			CheckError();
		}
		
		IGLDevice::Integer SDLGLDevice::GetProgramInteger(UInteger shader,
														 Enum param) {
			SPADES_MARK_FUNCTION();
			GLint ret;
			CheckExistence(glGetProgramiv);
			switch(param){
				case DeleteStatus:
					glGetProgramiv(shader, GL_DELETE_STATUS, &ret);
					break;
				case LinkStatus:
					glGetProgramiv(shader, GL_LINK_STATUS, &ret);
					break;
				case ValidateStatus:
					glGetProgramiv(shader, GL_VALIDATE_STATUS, &ret);
					break;
				case InfoLogLength:
					glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &ret);
					break;
				default:
					SPInvalidEnum("param", param);
			}
			CheckError();
			return ret;
		}
		
		void SDLGLDevice::GetProgramInfoLog(UInteger p, Sizei bufferSize,
										   Sizei *length, char *outString) {
			CheckExistence(glGetProgramInfoLog);
			glGetProgramInfoLog(p, bufferSize, (GLsizei *)length,
								(GLchar *)outString);
			CheckError();
		}
		
		IGLDevice::UInteger SDLGLDevice::CreateProgram() {
			CheckExistence(glCreateProgram);
			return glCreateProgram();
		}
		
		void SDLGLDevice::AttachShader(UInteger program, UInteger shader){
			CheckExistence(glAttachShader);
			glAttachShader(program, shader);
			CheckError();
		}
		
		void SDLGLDevice::DetachShader(UInteger program, UInteger shader){
			CheckExistence(glDetachShader);
			glDetachShader(program, shader);
			CheckError();
		}
		
		void SDLGLDevice::LinkProgram(UInteger program) {
			CheckExistence(glLinkProgram);
			glLinkProgram(program);
			CheckError();
		}
		
		void SDLGLDevice::UseProgram(UInteger program) {
			CheckExistence(glUseProgram);
			glUseProgram(program);
			CheckError();
		}
		
		void SDLGLDevice::DeleteProgram(UInteger program) {
			CheckExistence(glDeleteProgram);
			glDeleteProgram(program);
			CheckError();
		}
		
		void SDLGLDevice::ValidateProgram(UInteger program) {
			CheckExistence(glValidateProgram);
			glValidateProgram(program);
			CheckError();
		}
		
		IGLDevice::Integer SDLGLDevice::GetAttribLocation(UInteger program, const char *name){
			CheckExistence(glGetAttribLocation);
			return glGetAttribLocation(program, name);
		}
		
		void SDLGLDevice::BindAttribLocation(UInteger program, UInteger index, const char *name) {
			CheckExistence(glBindAttribLocation);
			glBindAttribLocation(program, index, name);
			CheckError();
		}
		
		IGLDevice::Integer SDLGLDevice::GetUniformLocation(UInteger program, const char *name) {
			CheckExistence(glGetUniformLocation);
			return glGetUniformLocation(program, name);
		}
		
		void SDLGLDevice::Uniform(Integer loc, Float x) {
			CheckExistence(glUniform1f);
			glUniform1f(loc, x);
			CheckError();
		}
		void SDLGLDevice::Uniform(Integer loc, Float x, Float y) {
			CheckExistence(glUniform2f);
			glUniform2f(loc, x, y);
			CheckError();
		}
		void SDLGLDevice::Uniform(Integer loc, Float x, Float y, Float z) {
			CheckExistence(glUniform3f);
			glUniform3f(loc, x, y, z);
			CheckError();
		}
		void SDLGLDevice::Uniform(Integer loc, Float x, Float y, Float z, Float w) {
			CheckExistence(glUniform4f);
			glUniform4f(loc, x, y, z, w);
			CheckError();
		}
		
		void SDLGLDevice::Uniform(Integer loc, Integer x) {
			CheckExistence(glUniform1i);
			glUniform1i(loc, x);
			CheckError();
		}
		void SDLGLDevice::Uniform(Integer loc, Integer x, Integer y) {
			CheckExistence(glUniform2i);
			glUniform2i(loc, x, y);
			CheckError();
		}
		void SDLGLDevice::Uniform(Integer loc, Integer x, Integer y, Integer z) {
			CheckExistence(glUniform3i);
			glUniform3i(loc, x, y, z);
			CheckError();
		}
		void SDLGLDevice::Uniform(Integer loc, Integer x, Integer y, Integer z, Integer w) {
			CheckExistence(glUniform4i);
			glUniform4i(loc, x, y, z, w);
			CheckError();
		}
		void SDLGLDevice::Uniform(Integer loc, bool transpose, const spades::Matrix4 &mat) {
			CheckExistence(glUniformMatrix4fv);
			glUniformMatrix4fv(loc, 1, transpose ? GL_TRUE : GL_FALSE,
							   mat.m);
			CheckError();
		}
		
		GLenum SDLGLDevice::parseFramebufferTarget(Enum v){
			SPADES_MARK_FUNCTION_DEBUG();
			switch(v){
				case draw::IGLDevice::Framebuffer:
					return GL_FRAMEBUFFER;
				case draw::IGLDevice::ReadFramebuffer:
					return GL_READ_FRAMEBUFFER;
				case draw::IGLDevice::DrawFramebuffer:
					return GL_DRAW_FRAMEBUFFER;
				default:
					SPInvalidEnum("v", v);
			}
		}
		
		IGLDevice::UInteger SDLGLDevice::GenFramebuffer() {
			GLuint v;
			CheckExistence(glGenFramebuffers);
			glGenFramebuffers(1, &v);
			return (IGLDevice::UInteger)v;
		}
		void SDLGLDevice::BindFramebuffer(Enum target,
										  UInteger framebuffer) {
			CheckExistence(glBindFramebuffer);
			glBindFramebuffer(parseFramebufferTarget(target),
							  framebuffer);
			CheckError();
		}
		void SDLGLDevice::DeleteFramebuffer(UInteger fb){
			CheckExistence(glDeleteFramebuffers);
			glDeleteFramebuffers(1, &fb);
			CheckError();
		}
		void SDLGLDevice::FramebufferTexture2D(Enum target,
											   Enum attachment,
											   Enum texTarget,
											   UInteger texture,
											   Integer level) {
			SPADES_MARK_FUNCTION_DEBUG();
			
			GLenum a;
			switch(attachment){
				case draw::IGLDevice::ColorAttachment0:
					a = GL_COLOR_ATTACHMENT0;
					break;
				case draw::IGLDevice::ColorAttachment1:
					a = GL_COLOR_ATTACHMENT1;
					break;
				case draw::IGLDevice::ColorAttachment2:
					a = GL_COLOR_ATTACHMENT2;
					break;
				case draw::IGLDevice::ColorAttachment3:
					a = GL_COLOR_ATTACHMENT3;
					break;
				case draw::IGLDevice::ColorAttachment4:
					a = GL_COLOR_ATTACHMENT4;
					break;
				case draw::IGLDevice::ColorAttachment5:
					a = GL_COLOR_ATTACHMENT5;
					break;
				case draw::IGLDevice::ColorAttachment6:
					a = GL_COLOR_ATTACHMENT6;
					break;
				case draw::IGLDevice::ColorAttachment7:
					a = GL_COLOR_ATTACHMENT7;
					break;
				case draw::IGLDevice::DepthAttachment:
					a = GL_DEPTH_ATTACHMENT;
					break;
				case draw::IGLDevice::StencilAttachment:
					a = GL_STENCIL_ATTACHMENT;
					break;
				default:
					SPInvalidEnum("attachment",attachment);
			}
			CheckExistence(glFramebufferTexture2D);
			glFramebufferTexture2D(parseFramebufferTarget(target),
								   a, parseTextureTarget(texTarget),
								   texture, level);
			CheckError();
		}
		
		void SDLGLDevice::BlitFramebuffer(Integer srcX0,
										  Integer srcY0,
										  Integer srcX1,
										  Integer srcY1,
										  Integer dstX0,
										  Integer dstY0,
										  Integer dstX1,
										  Integer dstY1,
										  UInteger mask,
										  Enum filter){
			SPADES_MARK_FUNCTION_DEBUG();
			
			GLenum flt;
			switch(filter){
				case draw::IGLDevice::Linear:
					flt = GL_LINEAR;
					break;
				case draw::IGLDevice::Nearest:
					flt = GL_NEAREST;
					break;
				default:
					SPInvalidEnum("filter", filter);
			}
			
			GLbitfield m = 0;
			if(mask & ColorBufferBit)
				m |= GL_COLOR_BUFFER_BIT;
			if(mask & DepthBufferBit)
				m |= GL_DEPTH_BUFFER_BIT;
			if(mask & StencilBufferBit)
				m |= GL_STENCIL_BUFFER_BIT;
			CheckExistence(glBlitFramebuffer);
			glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
							  dstX0, dstY0, dstX1, dstY1,
							  m, flt);
			CheckError();
		}
		
		GLenum SDLGLDevice::parseRenderbufferTarget(Enum e){
			SPADES_MARK_FUNCTION_DEBUG();
			switch(e){
				case draw::IGLDevice::Renderbuffer:
					return GL_RENDERBUFFER;
				default:
					SPInvalidEnum("e", e);
			}
		}
		
		IGLDevice::UInteger SDLGLDevice::GenRenderbuffer() {
			GLuint v;
			CheckExistence(glGenRenderbuffers);
			glGenRenderbuffers(1, &v);
			CheckError();
			return v;
		}
		void SDLGLDevice::DeleteRenderbuffer(UInteger v){
			CheckExistence(glDeleteRenderbuffers);
			glDeleteRenderbuffers(1, &v);
			CheckError();
		}
		void SDLGLDevice::BindRenderbuffer(Enum target, UInteger v){
			SPADES_MARK_FUNCTION_DEBUG();
			CheckExistence(glBindRenderbuffer);
			glBindRenderbuffer(parseRenderbufferTarget(target),
							   v);
			CheckError();
			
		}
		void SDLGLDevice::RenderbufferStorage(Enum target,
											  Enum intFormat,
											  Sizei width,
											  Sizei height){
			glRenderbufferStorage(parseRenderbufferTarget(target),
								  parseTextureInternalFormat(intFormat),
								  width, height);
			CheckError();
		}
		void SDLGLDevice::RenderbufferStorage(Enum target,
											  Sizei samples,
											  Enum intFormat,
											  Sizei width, Sizei height){
			CheckExistence(glRenderbufferStorage);
			glRenderbufferStorageMultisample(parseRenderbufferTarget(target),
											 samples,
											 parseTextureInternalFormat(intFormat),
											 width, height);
			CheckError();
		}
		void SDLGLDevice::FramebufferRenderbuffer(Enum target,
												  Enum attachment, Enum rbTarget, UInteger rb) {
			
			GLenum a;
			switch(attachment){
				case draw::IGLDevice::ColorAttachment0:
					a = GL_COLOR_ATTACHMENT0;
					break;
				case draw::IGLDevice::ColorAttachment1:
					a = GL_COLOR_ATTACHMENT1;
					break;
				case draw::IGLDevice::ColorAttachment2:
					a = GL_COLOR_ATTACHMENT2;
					break;
				case draw::IGLDevice::ColorAttachment3:
					a = GL_COLOR_ATTACHMENT3;
					break;
				case draw::IGLDevice::ColorAttachment4:
					a = GL_COLOR_ATTACHMENT4;
					break;
				case draw::IGLDevice::ColorAttachment5:
					a = GL_COLOR_ATTACHMENT5;
					break;
				case draw::IGLDevice::ColorAttachment6:
					a = GL_COLOR_ATTACHMENT6;
					break;
				case draw::IGLDevice::ColorAttachment7:
					a = GL_COLOR_ATTACHMENT7;
					break;
				case draw::IGLDevice::DepthAttachment:
					a = GL_DEPTH_ATTACHMENT;
					break;
				case draw::IGLDevice::StencilAttachment:
					a = GL_STENCIL_ATTACHMENT;
					break;
				default:
					SPInvalidEnum("attachment",attachment);
			}
			CheckExistence(glFramebufferRenderbuffer);
			glFramebufferRenderbuffer(parseFramebufferTarget(target),
									  a, parseRenderbufferTarget(rbTarget),
									  rb);
			CheckError();
		}
		
		void SDLGLDevice::ReadPixels(Integer x,
									 Integer y,
									 Sizei width,
									 Sizei height,
									 Enum format,
									 Enum type, void *data) {
			CheckExistence(glReadPixels);
			glReadPixels(x, y, width, height,
						 parseTextureFormat(format),
						 parseType(type), data);
			CheckError();
		}
		
		
		IGLDevice::Integer SDLGLDevice::ScreenWidth() {
			return surface->w;
		}
		
		IGLDevice::Integer SDLGLDevice::ScreenHeight() {
			return surface->h;
		}
		
	}
}
