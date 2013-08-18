//
//  SDLGLDevice.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Draw/IGLDevice.h"
#include "../Imports/OpenGL.h"
#include "../Imports/SDL.h"

namespace spades {
	namespace gui {
		class SDLGLDevice: public draw::IGLDevice {
			SDL_Surface *surface;
		public:
			SDLGLDevice(SDL_Surface *);
			
			virtual void DepthRange(Float near, Float far);
			virtual void Viewport(Integer x, Integer y,
								  Sizei width, Sizei height);
			
			virtual void ClearDepth(Float);
			virtual void ClearColor(Float, Float, Float, Float);
			virtual void Clear(Enum);
			
			virtual void Finish();
			virtual void Flush();
			
			virtual void DepthMask(bool);
			virtual void ColorMask(bool r, bool g, bool b, bool a);
			
			virtual void Enable(Enum state, bool);
			
			virtual Integer GetInteger(Enum type);
			
			virtual const char *GetString(Enum type);
			virtual const char *GetIndexedString(Enum type, UInteger);
			
			virtual void BlendEquation(Enum mode);
			virtual void BlendEquation(Enum rgb, Enum alpha);
			virtual void BlendFunc(Enum src, Enum dest);
			virtual void BlendFunc(Enum srcRgb, Enum destRgb,
								   Enum srcAlpha, Enum destAlpha);
			virtual void BlendColor(Float r, Float g, Float b, Float a);
			virtual void DepthFunc(Enum);
			virtual void LineWidth(Float);
			
			virtual UInteger GenBuffer();
			virtual void DeleteBuffer(UInteger);
			virtual void BindBuffer(Enum, UInteger);
			
			virtual void BufferData(Enum target,
									Sizei size,
									const void *data,
									Enum usage);
			virtual void BufferSubData(Enum target,
									   Sizei offset,
									   Sizei size,
									   const void *data);
			
			virtual UInteger GenTexture();
			virtual void DeleteTexture(UInteger);
			
			virtual void ActiveTexture(UInteger stage);
			virtual void BindTexture(Enum, UInteger);
			virtual void TexParamater(Enum target,
									  Enum paramater,
									  Enum value);
			virtual void TexImage2D(Enum target,
									Integer level,
									Enum internalFormat,
									Sizei width,
									Sizei height,
									Integer border,
									Enum format,
									Enum type,
									const void *data);
			virtual void TexImage3D(Enum target,
									Integer level,
									Enum internalFormat,
									Sizei width,
									Sizei height,
									Sizei depth,
									Integer border,
									Enum format,
									Enum type,
									const void *data);
			virtual void TexSubImage2D(Enum target,
									   Integer level,
									   Integer x,
									   Integer y,
									   Sizei width,
									   Sizei height,
									   Enum format,
									   Enum type,
									   const void *data);
			virtual void TexSubImage3D(Enum target,
									   Integer level,
									   Integer x,
									   Integer y,
									   Integer z,
									   Sizei width,
									   Sizei height,
									   Sizei depth,
									   Enum format,
									   Enum type,
									   const void *data);
			virtual void CopyTexSubImage2D(Enum target,
										   Integer level,
										   Integer destinationX,
										   Integer destinationY,
										   Integer srcX,
										   Integer srcY,
										   Sizei width,
										   Sizei height);
			virtual void GenerateMipmap(Enum target);
			
			virtual void VertexAttrib(UInteger index, Float);
			virtual void VertexAttrib(UInteger index, Float, Float);
			virtual void VertexAttrib(UInteger index, Float, Float, Float);
			virtual void VertexAttrib(UInteger index, Float, Float, Float, Float);
			
			virtual void VertexAttribPointer(UInteger index, Integer size,
											 Enum type, bool normalized,
											 Sizei stride, const void *);
			virtual void VertexAttribIPointer(UInteger index, Integer size,
											 Enum type, 
											 Sizei stride, const void *);
			virtual void EnableVertexAttribArray(UInteger index, bool);
			virtual void VertexAttribDivisor(UInteger index, UInteger divisor);
			
			virtual void DrawArrays(Enum mode, Integer first, Sizei count);
			virtual void DrawElements(Enum mode, Sizei count, Enum type, const void *indices);
			virtual void DrawArraysInstanced(Enum mode, Integer first, Sizei count,
											 Sizei instances);
			virtual void DrawElementsInstanced(Enum mode, Sizei count, Enum type, const void *indices,
											   Sizei instances);

			
			virtual UInteger CreateShader(Enum type);
			virtual void ShaderSource(UInteger shader, Sizei count,
									  const char **string, const int *len);
			virtual void CompileShader(UInteger);
			virtual void DeleteShader(UInteger);
			virtual Integer GetShaderInteger(UInteger shader, Enum param);
			virtual void GetShaderInfoLog(UInteger shader, Sizei bufferSize,
										  Sizei *length, char *outString);
			virtual Integer GetProgramInteger(UInteger program, Enum param);
			virtual void GetProgramInfoLog(UInteger program, Sizei bufferSize,
										   Sizei *length, char *outString);
			
			virtual UInteger CreateProgram();
			virtual void AttachShader(UInteger program, UInteger shader);
			virtual void DetachShader(UInteger program, UInteger shader);
			virtual void LinkProgram(UInteger program);
			virtual void UseProgram(UInteger program);
			virtual void DeleteProgram(UInteger program);
			virtual void ValidateProgram(UInteger program);
			virtual Integer GetAttribLocation(UInteger program, const char *name);
			virtual void BindAttribLocation(UInteger program, UInteger index, const char *name);
			virtual Integer GetUniformLocation(UInteger program, const char *name);
			virtual void Uniform(Integer loc, Float);
			virtual void Uniform(Integer loc, Float, Float);
			virtual void Uniform(Integer loc, Float, Float, Float);
			virtual void Uniform(Integer loc, Float, Float, Float, Float);
			virtual void Uniform(Integer loc, Integer);
			virtual void Uniform(Integer loc, Integer, Integer);
			virtual void Uniform(Integer loc, Integer, Integer, Integer);
			virtual void Uniform(Integer loc, Integer, Integer, Integer, Integer);
			virtual void Uniform(Integer loc, bool transpose, const Matrix4&);
			
			virtual UInteger GenRenderbuffer();
			virtual void DeleteRenderbuffer(UInteger);
			virtual void BindRenderbuffer(Enum target, UInteger);
			virtual void RenderbufferStorage(Enum target, Enum internalFormat, Sizei width, Sizei height);
			virtual void RenderbufferStorage(Enum target,  Sizei samples, Enum internalFormat, Sizei width, Sizei height);
			
			virtual UInteger GenFramebuffer();
			virtual void BindFramebuffer(Enum target, UInteger framebuffer);
			virtual void DeleteFramebuffer(UInteger);
			virtual void FramebufferTexture2D(Enum target, Enum attachment, Enum texTarget, UInteger texture, Integer level);
			virtual void FramebufferRenderbuffer(Enum target, Enum attachment, Enum renderbufferTarget, UInteger renderbuffer);
			virtual void BlitFramebuffer(Integer srcX0,
										 Integer srcY0,
										 Integer srcX1,
										 Integer srcY1,
										 Integer dstX0,
										 Integer dstY0,
										 Integer dstX1,
										 Integer dstY1,
										 UInteger mask,
										 Enum filter);
			
			virtual void ReadPixels(Integer x,
								   Integer y,
								   Sizei width,
								   Sizei height,
								   Enum format,
								   Enum type,
								   void *data);
			
			virtual Integer ScreenWidth();
			virtual Integer ScreenHeight();
			
			virtual void Swap();
			
		private:
			static GLenum parseBlendEquation(Enum);
			static GLenum parseBlendFunction(Enum);
			static GLenum parseBufferTarget(Enum);
			static GLenum parseTextureTarget(Enum);
			static GLenum parseTextureInternalFormat(Enum);
			static GLenum parseTextureFormat(Enum);
			static GLenum parseType(Enum);
			static GLenum parseFramebufferTarget(Enum);
			static GLenum parseRenderbufferTarget(Enum);
		};
	}
}

