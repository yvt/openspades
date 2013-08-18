//
//  GLFramebufferManager.h
//  OpenSpades
//
//  Created by yvt on 7/21/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLFramebufferManager {
		public:
			class BufferHandle {
				GLFramebufferManager *manager;
				size_t bufferIndex;
				bool valid;
			public:
				BufferHandle();
				BufferHandle(GLFramebufferManager *manager, size_t index);
				BufferHandle(const BufferHandle&);
				~BufferHandle();
				
				void operator =(const BufferHandle&);
				
				bool IsValid()const{return valid;}
				void Release();
				IGLDevice::UInteger GetFramebuffer();
				IGLDevice::UInteger GetTexture();
				int GetWidth();
				int GetHeight();
				
				GLFramebufferManager *GetManager() {
					return manager;
				}
			};
		private:
			IGLDevice *device;
			
			struct Buffer {
				IGLDevice::UInteger framebuffer;
				IGLDevice::UInteger texture;
				int refCount;
				int w, h;
				bool alpha;
			};
			
			bool useMultisample;
			
			IGLDevice::UInteger multisampledFramebuffer;
			
			// for multisample
			IGLDevice::UInteger multisampledColorRenderbuffer;
			IGLDevice::UInteger multisampledDepthRenderbuffer;
			
			// common
			IGLDevice::UInteger renderFramebuffer;
			IGLDevice::UInteger renderColorTexture;
			IGLDevice::UInteger renderDepthTexture;
			
			std::vector<Buffer> buffers;
			
		public:
			GLFramebufferManager(IGLDevice *);
			~GLFramebufferManager();
			
			/** setups device for scene rendering. */
			void PrepareSceneRendering();
			
			BufferHandle PrepareForWaterRendering(IGLDevice::UInteger tempFb);
			BufferHandle StartPostProcessing();
			
			void MakeSureAllBuffersReleased();
			
			IGLDevice::UInteger GetDepthTexture(){
				return renderDepthTexture;
			}
			BufferHandle CreateBufferHandle(int w=-1, int h=-1, bool alpha=false);
		};
		
		
		// name is too long, so shorten it!
		typedef GLFramebufferManager::BufferHandle GLColorBuffer;
	}
}