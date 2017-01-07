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

#pragma once

#include <Imports/OpenGL.h>
#include <Imports/SDL.h>

#include <Draw/IGLDevice.h>

namespace spades {
	namespace gui {
		class SDLGLDevice : public draw::IGLDevice {
			SDL_Window *window;
			SDL_GLContext context;
			int w, h;

		protected:
			~SDLGLDevice();

		public:
			SDLGLDevice(SDL_Window *);

			void DepthRange(Float near, Float far) override;
			void Viewport(Integer x, Integer y, Sizei width, Sizei height) override;

			void ClearDepth(Float) override;
			void ClearColor(Float, Float, Float, Float) override;
			void Clear(Enum) override;

			void Finish() override;
			void Flush() override;

			void DepthMask(bool) override;
			void ColorMask(bool r, bool g, bool b, bool a) override;

			void FrontFace(Enum) override;
			void Enable(Enum state, bool) override;

			Integer GetInteger(Enum type) override;

			const char *GetString(Enum type) override;
			const char *GetIndexedString(Enum type, UInteger) override;

			void BlendEquation(Enum mode) override;
			void BlendEquation(Enum rgb, Enum alpha) override;
			void BlendFunc(Enum src, Enum dest) override;
			void BlendFunc(Enum srcRgb, Enum destRgb, Enum srcAlpha, Enum destAlpha) override;
			void BlendColor(Float r, Float g, Float b, Float a) override;
			void DepthFunc(Enum) override;
			void LineWidth(Float) override;

			UInteger GenBuffer() override;
			void DeleteBuffer(UInteger) override;
			void BindBuffer(Enum, UInteger) override;

			void *MapBuffer(Enum target, Enum access) override;
			void UnmapBuffer(Enum target) override;

			void BufferData(Enum target, Sizei size, const void *data, Enum usage) override;
			void BufferSubData(Enum target, Sizei offset, Sizei size, const void *data) override;

			UInteger GenQuery() override;
			void DeleteQuery(UInteger) override;
			void BeginQuery(Enum target, UInteger query) override;
			void EndQuery(Enum target) override;
			UInteger GetQueryObjectUInteger(UInteger query, Enum pname) override;
			UInteger64 GetQueryObjectUInteger64(UInteger query, Enum pname) override;
			void BeginConditionalRender(UInteger query, Enum) override;
			void EndConditionalRender() override;

			UInteger GenTexture() override;
			void DeleteTexture(UInteger) override;

			void ActiveTexture(UInteger stage) override;
			void BindTexture(Enum, UInteger) override;
			void TexParamater(Enum target, Enum paramater, Enum value) override;
			void TexParamater(Enum target, Enum paramater, float value) override;
			void TexImage2D(Enum target, Integer level, Enum internalFormat, Sizei width,
			                Sizei height, Integer border, Enum format, Enum type,
			                const void *data) override;
			void TexImage3D(Enum target, Integer level, Enum internalFormat, Sizei width,
			                Sizei height, Sizei depth, Integer border, Enum format, Enum type,
			                const void *data) override;
			void TexSubImage2D(Enum target, Integer level, Integer x, Integer y, Sizei width,
			                   Sizei height, Enum format, Enum type, const void *data) override;
			void TexSubImage3D(Enum target, Integer level, Integer x, Integer y, Integer z,
			                   Sizei width, Sizei height, Sizei depth, Enum format, Enum type,
			                   const void *data) override;
			void CopyTexSubImage2D(Enum target, Integer level, Integer destinationX,
			                       Integer destinationY, Integer srcX, Integer srcY, Sizei width,
			                       Sizei height) override;
			void GenerateMipmap(Enum target) override;

			void VertexAttrib(UInteger index, Float) override;
			void VertexAttrib(UInteger index, Float, Float) override;
			void VertexAttrib(UInteger index, Float, Float, Float) override;
			void VertexAttrib(UInteger index, Float, Float, Float, Float) override;

			void VertexAttribPointer(UInteger index, Integer size, Enum type, bool normalized,
			                         Sizei stride, const void *) override;
			void VertexAttribIPointer(UInteger index, Integer size, Enum type, Sizei stride,
			                          const void *) override;
			void EnableVertexAttribArray(UInteger index, bool) override;
			void VertexAttribDivisor(UInteger index, UInteger divisor) override;

			void DrawArrays(Enum mode, Integer first, Sizei count) override;
			void DrawElements(Enum mode, Sizei count, Enum type, const void *indices) override;
			void DrawArraysInstanced(Enum mode, Integer first, Sizei count,
			                         Sizei instances) override;
			void DrawElementsInstanced(Enum mode, Sizei count, Enum type, const void *indices,
			                           Sizei instances) override;

			UInteger CreateShader(Enum type) override;
			void ShaderSource(UInteger shader, Sizei count, const char **string,
			                  const int *len) override;
			void CompileShader(UInteger) override;
			void DeleteShader(UInteger) override;
			Integer GetShaderInteger(UInteger shader, Enum param) override;
			void GetShaderInfoLog(UInteger shader, Sizei bufferSize, Sizei *length,
			                      char *outString) override;
			Integer GetProgramInteger(UInteger program, Enum param) override;
			void GetProgramInfoLog(UInteger program, Sizei bufferSize, Sizei *length,
			                       char *outString) override;

			UInteger CreateProgram() override;
			void AttachShader(UInteger program, UInteger shader) override;
			void DetachShader(UInteger program, UInteger shader) override;
			void LinkProgram(UInteger program) override;
			void UseProgram(UInteger program) override;
			void DeleteProgram(UInteger program) override;
			void ValidateProgram(UInteger program) override;
			Integer GetAttribLocation(UInteger program, const char *name) override;
			void BindAttribLocation(UInteger program, UInteger index, const char *name) override;
			Integer GetUniformLocation(UInteger program, const char *name) override;
			void Uniform(Integer loc, Float) override;
			void Uniform(Integer loc, Float, Float) override;
			void Uniform(Integer loc, Float, Float, Float) override;
			void Uniform(Integer loc, Float, Float, Float, Float) override;
			void Uniform(Integer loc, Integer) override;
			void Uniform(Integer loc, Integer, Integer) override;
			void Uniform(Integer loc, Integer, Integer, Integer) override;
			void Uniform(Integer loc, Integer, Integer, Integer, Integer) override;
			void Uniform(Integer loc, bool transpose, const Matrix4 &) override;

			UInteger GenRenderbuffer() override;
			void DeleteRenderbuffer(UInteger) override;
			void BindRenderbuffer(Enum target, UInteger) override;
			void RenderbufferStorage(Enum target, Enum internalFormat, Sizei width,
			                         Sizei height) override;
			void RenderbufferStorage(Enum target, Sizei samples, Enum internalFormat, Sizei width,
			                         Sizei height) override;

			UInteger GenFramebuffer() override;
			void BindFramebuffer(Enum target, UInteger framebuffer) override;
			void DeleteFramebuffer(UInteger) override;
			void FramebufferTexture2D(Enum target, Enum attachment, Enum texTarget,
			                          UInteger texture, Integer level) override;
			void FramebufferRenderbuffer(Enum target, Enum attachment, Enum renderbufferTarget,
			                             UInteger renderbuffer) override;
			void BlitFramebuffer(Integer srcX0, Integer srcY0, Integer srcX1, Integer srcY1,
			                     Integer dstX0, Integer dstY0, Integer dstX1, Integer dstY1,
			                     UInteger mask, Enum filter) override;
			Enum CheckFramebufferStatus(Enum target) override;

			void ReadPixels(Integer x, Integer y, Sizei width, Sizei height, Enum format, Enum type,
			                void *data) override;

			Integer ScreenWidth() override;
			Integer ScreenHeight() override;

			void Swap() override;

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
