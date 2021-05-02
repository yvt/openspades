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

#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLSettings;

		class GLFramebufferManager {
		public:
			class BufferHandle {
				GLFramebufferManager *manager;
				size_t bufferIndex;
				bool valid;

			public:
				BufferHandle();
				BufferHandle(GLFramebufferManager *manager, size_t index);
				BufferHandle(const BufferHandle &);
				~BufferHandle();

				void operator=(const BufferHandle &);

				bool IsValid() const { return valid; }
				void Release();
				IGLDevice::UInteger GetFramebuffer();
				IGLDevice::UInteger GetTexture();
				int GetWidth();
				int GetHeight();

				IGLDevice::Enum GetInternalFormat();

				GLFramebufferManager *GetManager() { return manager; }
			};

		private:
			IGLDevice &device;
			GLSettings &settings;

			struct Buffer {
				IGLDevice::UInteger framebuffer;
				IGLDevice::UInteger texture;
				int refCount;
				int w, h;
				IGLDevice::Enum internalFormat;
			};

			bool useMultisample;
			bool useHighPrec;
			bool useHdr;

			bool doingPostProcessing;

			int renderWidth;
			int renderHeight;

			IGLDevice::UInteger multisampledFramebuffer = 0;

			// for multisample
			IGLDevice::UInteger multisampledColorRenderbuffer = 0;
			IGLDevice::UInteger multisampledDepthRenderbuffer = 0;

			// common
			IGLDevice::UInteger renderFramebuffer = 0;
			IGLDevice::UInteger renderColorTexture = 0;
			IGLDevice::UInteger renderDepthTexture = 0;

			IGLDevice::UInteger renderFramebufferWithoutDepth = 0;

			IGLDevice::Enum fbInternalFormat;

			IGLDevice::UInteger mirrorFramebuffer = 0;
			IGLDevice::UInteger mirrorColorTexture = 0;
			IGLDevice::UInteger mirrorDepthTexture = 0;

			std::vector<Buffer> buffers;

		public:
			GLFramebufferManager(IGLDevice &, GLSettings &, int renderWidth, int renderHeight);
			~GLFramebufferManager();

			/** setups device for scene rendering. */
			void PrepareSceneRendering();

			BufferHandle PrepareForWaterRendering(IGLDevice::UInteger tempFb,
			                                      IGLDevice::UInteger tempDepthTex);
			BufferHandle StartPostProcessing();

			void MakeSureAllBuffersReleased();

			IGLDevice::UInteger GetDepthTexture() { return renderDepthTexture; }
			IGLDevice::Enum GetMainInternalFormat() { return fbInternalFormat; }


			/** 
			 * Creates BufferHandle with a given size and format.
			 * Might clobber the current framebuffer and texture bindings.
			 */
			BufferHandle CreateBufferHandle(int w = -1, int h = -1, bool alpha = false);

			/**
			 * Creates BufferHandle with a given size and format.
			 * Might clobber the current framebuffer and texture bindings.
			 */
			BufferHandle CreateBufferHandle(int w, int h, IGLDevice::Enum internalFormat);

			void CopyToMirrorTexture(IGLDevice::UInteger fb = 0);
			void ClearMirrorTexture(Vector3);
			IGLDevice::UInteger GetMirrorTexture() { return mirrorColorTexture; }
			IGLDevice::UInteger GetMirrorDepthTexture() { return mirrorDepthTexture; }
		};

		// name is too long, so shorten it!
		typedef GLFramebufferManager::BufferHandle GLColorBuffer;
	} // namespace draw
} // namespace spades
