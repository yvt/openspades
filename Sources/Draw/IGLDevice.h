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

#include <cstdlib> // for integer types

#include <Core/Math.h>
#include <Core/RefCountedObject.h>

#undef Always
#undef None

namespace spades {
	namespace draw {
		class IGLDevice : public RefCountedObject {
		protected:
			virtual ~IGLDevice() {}

		public:
			enum Enum {

				// datatype
				Int,
				UnsignedInt,
				Short,
				UnsignedShort,
				Byte,
				UnsignedByte,
				FloatType,
				UnsignedShort5551,
				UnsignedShort1555Rev,
				UnsignedInt2101010Rev,

				// Front face
				CW,
				CCW,

				// State
				DepthTest,
				CullFace,
				Blend,
				Multisample,
				FramebufferSRGB,

				// Parameters
				FramebufferBinding,

				// String query
				Vendor,
				Renderer,
				Version,
				ShadingLanguageVersion,
				Extensions,

				// Blend equation
				Add,
				Subtract,
				ReverseSubtract,
				MinOp,
				MaxOp,

				// Blend function
				Zero,
				One,
				SrcColor,
				DestColor,
				OneMinusSrcColor,
				OneMinusDestColor,
				SrcAlpha,
				DestAlpha,
				OneMinusSrcAlpha,
				OneMinusDestAlpha,
				ConstantColor,
				ConstantAlpha,
				OneMinusConstantColor,
				OneMinusConstantAlpha,

				// Depth function
				Never,
				Always,
				Less,
				LessOrEqual,
				Equal,
				Greater,
				GreaterOrEqual,
				NotEqual,

				// cull
				Front,
				Back,
				FrontAndBack,

				// Query Object
				SamplesPassed,
				AnySamplesPassed,

				QueryResult,
				QueryResultAvailable,

				// Conditional Render
				QueryWait,
				QueryNoWait,
				QueryByRegionWait,
				QueryByRegionNoWait,

				// Buffer target
				ArrayBuffer,
				ElementArrayBuffer,
				PixelPackBuffer,
				PixelUnpackBuffer,

				// Buffer usage
				StaticDraw,
				StreamDraw,
				DynamicDraw,

				// Buffer Map access
				ReadOnly,
				WriteOnly,
				ReadWrite,

				// Texture targets
				Texture2D,
				Texture3D,
				Texture2DArray,

				// Texture parameter
				TextureMinFilter,
				TextureMagFilter,
				TextureWrapS,
				TextureWrapT,
				TextureWrapR,
				TextureCompareMode,
				TextureCompareFunc,
				TextureMaxAnisotropy,

				// texture compare mode
				CompareRefToTexture,
				None,

				// Texture filter
				Nearest,
				Linear,
				NearestMipmapNearest,
				NearestMipmapLinear,
				LinearMipmapNearest,
				LinearMipmapLinear,

				// Texture internal format
				Red,
				RG,
				RGB,
				RGBA,
				BGRA,
				DepthComponent,
				DepthComponent24,
				StencilIndex,
				RGB10A2,
				RGB16F,
				RGBA16F,
				R16F,
				RGB5,
				RGB5A1,
				RGB8,
				RGBA8,
				SRGB8,
				SRGB8Alpha,

				// Texture wrap
				ClampToEdge,
				Repeat,

				// draw mode
				Points,
				LineStrip,
				LineLoop,
				Lines,
				TriangleStrip,
				TriangleFan,
				Triangles,

				// shader type
				VertexShader,
				GeometryShader,
				FragmentShader,

				// shader query
				ShaderType,
				DeleteStatus,
				CompileStatus,
				InfoLogLength,
				ShaderSourceLength,

				// program query
				/* DeleteStatus, */
				LinkStatus,
				ValidateStatus,
				/* InfoLogLength, */
				AttachedShaders,

				// renderbuffer target
				Renderbuffer,

				// framebuffer target
				Framebuffer,
				ReadFramebuffer,
				DrawFramebuffer,

				// framebuffer attachment
				ColorAttachment0,
				ColorAttachment1,
				ColorAttachment2,
				ColorAttachment3,
				ColorAttachment4,
				ColorAttachment5,
				ColorAttachment6,
				ColorAttachment7,
				DepthAttachment,
				StencilAttachment,

				// framebuffer status
				FramebufferComplete,
				FramebufferUndefined,
				FramebufferIncompleteAttachment,
				FramebufferIncompleteMissingAttachment,
				FramebufferIncompleteDrawBuffer,
				FramebufferIncompleteReadBuffer,
				FramebufferUnsupported,
				FramebufferIncompleteMultisample,
				FramebufferIncompleteLayerTargets,

				// EXT_timer_query
				TimeElapsed,

				ColorBufferBit = 1,
				DepthBufferBit = 2,
				StencilBufferBit = 4

			};
			typedef unsigned int UInteger;
			typedef int Integer;
			typedef float Float;
			typedef unsigned int Sizei;
			typedef std::uint64_t UInteger64;

			virtual void DepthRange(Float near, Float far) = 0;
			virtual void Viewport(Integer x, Integer y, Sizei width, Sizei height) = 0;

			virtual void ClearDepth(Float) = 0;
			virtual void ClearColor(Float, Float, Float, Float) = 0;
			virtual void Clear(Enum) = 0;

			virtual void DepthMask(bool) = 0;
			virtual void ColorMask(bool r, bool g, bool b, bool a) = 0;

			virtual void Finish() = 0;
			virtual void Flush() = 0;

			virtual void FrontFace(Enum) = 0;
			virtual void Enable(Enum state, bool) = 0;

			virtual const char *GetString(Enum type) = 0;
			virtual const char *GetIndexedString(Enum type, UInteger) = 0;

			virtual Integer GetInteger(Enum type) = 0;

			virtual void BlendEquation(Enum mode) = 0;
			virtual void BlendEquation(Enum rgb, Enum alpha) = 0;
			virtual void BlendFunc(Enum src, Enum dest) = 0;
			virtual void BlendFunc(Enum srcRgb, Enum destRgb, Enum srcAlpha, Enum destAlpha) = 0;
			virtual void BlendColor(Float r, Float g, Float b, Float a) = 0;
			virtual void DepthFunc(Enum) = 0;
			virtual void LineWidth(Float) = 0;

			virtual UInteger GenBuffer() = 0;
			virtual void DeleteBuffer(UInteger) = 0;
			virtual void BindBuffer(Enum, UInteger) = 0;

			virtual void BufferData(Enum target, Sizei size, const void *data, Enum usage) = 0;
			virtual void BufferSubData(Enum target, Sizei offset, Sizei size, const void *data) = 0;

			virtual UInteger GenQuery() = 0;
			virtual void DeleteQuery(UInteger) = 0;
			virtual void BeginQuery(Enum target, UInteger query) = 0;
			virtual void EndQuery(Enum target) = 0;
			virtual UInteger GetQueryObjectUInteger(UInteger query, Enum pname) = 0;
			virtual UInteger64 GetQueryObjectUInteger64(UInteger query, Enum pname) = 0;
			virtual void BeginConditionalRender(UInteger query, Enum mode) = 0;
			virtual void EndConditionalRender() = 0;

			virtual void *MapBuffer(Enum target, Enum access) = 0;
			virtual void UnmapBuffer(Enum target) = 0;

			virtual UInteger GenTexture() = 0;
			virtual void DeleteTexture(UInteger) = 0;

			virtual void ActiveTexture(UInteger stage) = 0;
			virtual void BindTexture(Enum, UInteger) = 0;
			virtual void TexParamater(Enum target, Enum paramater, Enum value) = 0;
			virtual void TexParamater(Enum target, Enum paramater, float value) = 0;
			virtual void TexImage2D(Enum target, Integer level, Enum internalFormat, Sizei width,
			                        Sizei height, Integer border, Enum format, Enum type,
			                        const void *data) = 0;
			virtual void TexImage3D(Enum target, Integer level, Enum internalFormat, Sizei width,
			                        Sizei height, Sizei depth, Integer border, Enum format,
			                        Enum type, const void *data) = 0;
			virtual void TexSubImage2D(Enum target, Integer level, Integer x, Integer y,
			                           Sizei width, Sizei height, Enum format, Enum type,
			                           const void *data) = 0;
			virtual void TexSubImage3D(Enum target, Integer level, Integer x, Integer y, Integer z,
			                           Sizei width, Sizei height, Sizei depth, Enum format,
			                           Enum type, const void *data) = 0;
			virtual void CopyTexSubImage2D(Enum target, Integer level, Integer destinationX,
			                               Integer destinationY, Integer srcX, Integer srcY,
			                               Sizei width, Sizei height) = 0;
			virtual void GenerateMipmap(Enum target) = 0;

			virtual void VertexAttrib(UInteger index, Float) = 0;
			virtual void VertexAttrib(UInteger index, Float, Float) = 0;
			virtual void VertexAttrib(UInteger index, Float, Float, Float) = 0;
			virtual void VertexAttrib(UInteger index, Float, Float, Float, Float) = 0;

			virtual void VertexAttribPointer(UInteger index, Integer size, Enum type,
			                                 bool normalized, Sizei stride, const void *) = 0;
			virtual void VertexAttribIPointer(UInteger index, Integer size, Enum type, Sizei stride,
			                                  const void *) = 0;
			virtual void EnableVertexAttribArray(UInteger index, bool) = 0;
			virtual void VertexAttribDivisor(UInteger index, UInteger divisor) = 0;

			virtual void DrawArrays(Enum mode, Integer first, Sizei count) = 0;
			virtual void DrawElements(Enum mode, Sizei count, Enum type, const void *indices) = 0;
			virtual void DrawArraysInstanced(Enum mode, Integer first, Sizei count,
			                                 Sizei instances) = 0;
			virtual void DrawElementsInstanced(Enum mode, Sizei count, Enum type,
			                                   const void *indices, Sizei instances) = 0;

			virtual UInteger CreateShader(Enum type) = 0;
			virtual void ShaderSource(UInteger shader, Sizei count, const char **string,
			                          const int *len) = 0;
			virtual void CompileShader(UInteger) = 0;
			virtual void DeleteShader(UInteger) = 0;
			virtual Integer GetShaderInteger(UInteger shader, Enum param) = 0;
			virtual void GetShaderInfoLog(UInteger shader, Sizei bufferSize, Sizei *length,
			                              char *outString) = 0;
			virtual Integer GetProgramInteger(UInteger program, Enum param) = 0;
			virtual void GetProgramInfoLog(UInteger program, Sizei bufferSize, Sizei *length,
			                               char *outString) = 0;

			virtual UInteger CreateProgram() = 0;
			virtual void AttachShader(UInteger program, UInteger shader) = 0;
			virtual void DetachShader(UInteger program, UInteger shader) = 0;
			virtual void LinkProgram(UInteger program) = 0;
			virtual void UseProgram(UInteger program) = 0;
			virtual void DeleteProgram(UInteger program) = 0;
			virtual void ValidateProgram(UInteger program) = 0;
			virtual Integer GetAttribLocation(UInteger program, const char *name) = 0;
			virtual void BindAttribLocation(UInteger program, UInteger index, const char *name) = 0;
			virtual Integer GetUniformLocation(UInteger program, const char *name) = 0;
			virtual void Uniform(Integer loc, Float) = 0;
			virtual void Uniform(Integer loc, Float, Float) = 0;
			virtual void Uniform(Integer loc, Float, Float, Float) = 0;
			virtual void Uniform(Integer loc, Float, Float, Float, Float) = 0;
			virtual void Uniform(Integer loc, Integer) = 0;
			virtual void Uniform(Integer loc, Integer, Integer) = 0;
			virtual void Uniform(Integer loc, Integer, Integer, Integer) = 0;
			virtual void Uniform(Integer loc, Integer, Integer, Integer, Integer) = 0;
			virtual void Uniform(Integer loc, bool transpose, const Matrix4 &) = 0;

			virtual UInteger GenRenderbuffer() = 0;
			virtual void DeleteRenderbuffer(UInteger) = 0;
			virtual void BindRenderbuffer(Enum target, UInteger) = 0;
			virtual void RenderbufferStorage(Enum target, Enum internalFormat, Sizei width,
			                                 Sizei height) = 0;
			virtual void RenderbufferStorage(Enum target, Sizei samples, Enum internalFormat,
			                                 Sizei width, Sizei height) = 0;

			virtual UInteger GenFramebuffer() = 0;
			virtual void BindFramebuffer(Enum target, UInteger framebuffer) = 0;
			virtual void DeleteFramebuffer(UInteger) = 0;
			virtual void FramebufferTexture2D(Enum target, Enum attachment, Enum texTarget,
			                                  UInteger texture, Integer level) = 0;
			virtual void FramebufferRenderbuffer(Enum target, Enum attachment,
			                                     Enum renderbufferTarget,
			                                     UInteger renderbuffer) = 0;
			virtual void BlitFramebuffer(Integer srcX0, Integer srcY0, Integer srcX1, Integer srcY1,
			                             Integer dstX0, Integer dstY0, Integer dstX1, Integer dstY1,
			                             UInteger mask, Enum filter) = 0;
			virtual Enum CheckFramebufferStatus(Enum target) = 0;

			virtual void ReadPixels(Integer x, Integer y, Sizei width, Sizei height, Enum format,
			                        Enum type, void *data) = 0;

			virtual Integer ScreenWidth() = 0;
			virtual Integer ScreenHeight() = 0;

			virtual void Swap() = 0;
		};
	}
}
