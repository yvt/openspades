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

#include "GLFramebufferManager.h"
#include "IGLDevice.h"
#include "../Core/Settings.h"
#include "../Core/Debug.h"
#include "../Core/Debug.h"
#include "../Core/Exception.h"

SPADES_SETTING(r_multisamples, "0");
SPADES_SETTING(r_depthBits, "24"); // TODO: use this value
SPADES_SETTING(r_colorBits, "");  // TOOD: use this value
SPADES_SETTING(r_srgb, "1");
SPADES_SETTING(r_blitFramebuffer, "1");

namespace spades {
	namespace draw {
		static void RaiseFBStatusError(IGLDevice::Enum status) {
			std::string type;
			switch(status){
				case IGLDevice::FramebufferComplete:
					type = "GL_FRAMEBUFFER_COMPLETE";
					break;
				case IGLDevice::FramebufferUndefined:
					type = "GL_FRAMEBUFFER_UNDEFINED";
					break;
				case IGLDevice::FramebufferIncompleteAttachment:
					type = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
					break;
				case IGLDevice::				FramebufferIncompleteMissingAttachment:
					type = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
					break;
				case IGLDevice::				FramebufferIncompleteDrawBuffer:
					type = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
					break;
				case IGLDevice::				FramebufferIncompleteReadBuffer:
					type = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
					break;
				case IGLDevice::				FramebufferIncompleteMultisample:
					type = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
					break;
				case IGLDevice::				FramebufferIncompleteLayerTargets:
					type = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
					break;
			}
			SPRaise("OpenGL Framebuffer completeness check failed: %s",
					type.c_str());
		}
		
		GLFramebufferManager::GLFramebufferManager(IGLDevice *dev):
		device(dev){
			SPADES_MARK_FUNCTION();
			
			SPLog("Initializing framebuffer manager");
			
			if((!r_blitFramebuffer) && r_multisamples) {
				SPLog("WARNING: Disabling MSAA: no support for MSAA when r_blitFramebuffer = 0");
				r_multisamples = 0;
			}
			
			useMultisample = (int)r_multisamples > 0;
			useHighPrec = true;
			
			if(useMultisample){
				SPLog("Multi-sample Antialiasing Enabled");
				
				// for multisample rendering, use
				// multisample renderbuffer for scene
				// rendering.
				
				multisampledFramebuffer = dev->GenFramebuffer();
				dev->BindFramebuffer(IGLDevice::Framebuffer,
									 multisampledFramebuffer);
			
				multisampledDepthRenderbuffer = dev->GenRenderbuffer();
				dev->BindRenderbuffer(IGLDevice::Renderbuffer,
									  multisampledDepthRenderbuffer);
				dev->RenderbufferStorage(IGLDevice::Renderbuffer,
										 (int)r_multisamples,
										 IGLDevice::DepthComponent24,
										 dev->ScreenWidth(),
										 dev->ScreenHeight());
				SPLog("MSAA Depth Buffer Allocated");
				
				dev->FramebufferRenderbuffer(IGLDevice::Framebuffer,
											 IGLDevice::DepthAttachment,
											 IGLDevice::Renderbuffer,
											 multisampledDepthRenderbuffer);
				
				multisampledColorRenderbuffer = dev->GenRenderbuffer();
				dev->BindRenderbuffer(IGLDevice::Renderbuffer,
									  multisampledColorRenderbuffer);
				if(r_srgb){
					SPLog("Creating MSAA Color Buffer with SRGB8_ALPHA");
					useHighPrec = false;
					dev->RenderbufferStorage(IGLDevice::Renderbuffer,
											 (int)r_multisamples,
											 IGLDevice::SRGB8Alpha,
											 dev->ScreenWidth(),
											 dev->ScreenHeight());
					
					SPLog("MSAA Color Buffer Allocated");
					
					dev->FramebufferRenderbuffer(IGLDevice::Framebuffer,
												 IGLDevice::ColorAttachment0,
												 IGLDevice::Renderbuffer,
												 multisampledColorRenderbuffer);
					IGLDevice::Enum status = dev->CheckFramebufferStatus(IGLDevice::Framebuffer);
					if(status != IGLDevice::FramebufferComplete) {
						RaiseFBStatusError(status);
					}
					SPLog("MSAA Framebuffer Allocated");
				}else{
					try{
						dev->RenderbufferStorage(IGLDevice::Renderbuffer,
												 (int)r_multisamples,
												 IGLDevice::RGB10A2,
												 dev->ScreenWidth(),
												 dev->ScreenHeight());
						SPLog("MSAA Color Buffer Allocated");
						
						dev->FramebufferRenderbuffer(IGLDevice::Framebuffer,
													 IGLDevice::ColorAttachment0,
													 IGLDevice::Renderbuffer,
													 multisampledColorRenderbuffer);
						IGLDevice::Enum status = dev->CheckFramebufferStatus(IGLDevice::Framebuffer);
						if(status != IGLDevice::FramebufferComplete) {
							RaiseFBStatusError(status);
						}
						SPLog("MSAA Framebuffer Allocated");
						
					}catch(...){
						SPLog("Renderbuffer creation failed: trying with RGB8");
						useHighPrec = false;
						dev->RenderbufferStorage(IGLDevice::Renderbuffer,
												 (int)r_multisamples,
												 IGLDevice::RGBA8,
												 dev->ScreenWidth(),
												 dev->ScreenHeight());
						
						SPLog("MSAA Color Buffer Allocated");
						
						dev->FramebufferRenderbuffer(IGLDevice::Framebuffer,
													 IGLDevice::ColorAttachment0,
													 IGLDevice::Renderbuffer,
													 multisampledColorRenderbuffer);
						IGLDevice::Enum status = dev->CheckFramebufferStatus(IGLDevice::Framebuffer);
						if(status != IGLDevice::FramebufferComplete) {
							RaiseFBStatusError(status);
						}
						SPLog("MSAA Framebuffer Allocated");
					}
				}
			}
			
			SPLog("Creating Non-MSAA Buffer");
			
			// in non-multisampled rendering,
			// we can directly draw into
			// texture.
			// in multisampled rendering,
			// we must first copy to non-multismapled
			// framebuffer to use it in shader as a texture.
			
			renderFramebuffer = dev->GenFramebuffer();
			dev->BindFramebuffer(IGLDevice::Framebuffer,
								 renderFramebuffer);
			
			renderDepthTexture = dev->GenTexture();
			dev->BindTexture(IGLDevice::Texture2D,
							 renderDepthTexture);
			dev->TexImage2D(IGLDevice::Texture2D,
							0,
							IGLDevice::DepthComponent24,
							dev->ScreenWidth(),
							dev->ScreenHeight(),
							0,
							IGLDevice::DepthComponent,
							IGLDevice::UnsignedInt, NULL);
			SPLog("Depth Buffer Allocated");
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMagFilter,
							  IGLDevice::Nearest);
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMinFilter,
							  IGLDevice::Nearest);
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapS,
							  IGLDevice::ClampToEdge);
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapT,
							  IGLDevice::ClampToEdge);
			
			dev->FramebufferTexture2D(IGLDevice::Framebuffer,
									  IGLDevice::DepthAttachment,
									  IGLDevice::Texture2D,
									  renderDepthTexture, 0);
			
			renderColorTexture = dev->GenTexture();
			dev->BindTexture(IGLDevice::Texture2D,
							 renderColorTexture);
			if(r_srgb){
				SPLog("Creating Non-MSAA SRGB buffer");
				useHighPrec = false;
				dev->TexImage2D(IGLDevice::Texture2D,
								0,
								IGLDevice::SRGB8Alpha,
								dev->ScreenWidth(),
								dev->ScreenHeight(),
								0,
								IGLDevice::RGBA,
								IGLDevice::UnsignedByte, NULL);
				SPLog("Color Buffer Allocated");
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureMagFilter,
								  IGLDevice::Linear);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureMinFilter,
								  IGLDevice::Linear);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureWrapS,
								  IGLDevice::ClampToEdge);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureWrapT,
								  IGLDevice::ClampToEdge);
				
				dev->FramebufferTexture2D(IGLDevice::Framebuffer,
										  IGLDevice::ColorAttachment0,
										  IGLDevice::Texture2D,
										  renderColorTexture, 0);
				
				IGLDevice::Enum status = dev->CheckFramebufferStatus(IGLDevice::Framebuffer);
				if(status != IGLDevice::FramebufferComplete) {
					RaiseFBStatusError(status);
				}
				SPLog("Framebuffer Created");
			}else{
				try{
					dev->TexImage2D(IGLDevice::Texture2D,
									0,
									IGLDevice::RGB10A2,
									dev->ScreenWidth(),
									dev->ScreenHeight(),
									0,
									IGLDevice::RGBA,
									IGLDevice::UnsignedByte, NULL);
					SPLog("Color Buffer Allocated");
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureMagFilter,
									  IGLDevice::Linear);
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureMinFilter,
									  IGLDevice::Linear);
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureWrapS,
									  IGLDevice::ClampToEdge);
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureWrapT,
									  IGLDevice::ClampToEdge);
					
					dev->FramebufferTexture2D(IGLDevice::Framebuffer,
											  IGLDevice::ColorAttachment0,
											  IGLDevice::Texture2D,
											  renderColorTexture, 0);
					
					IGLDevice::Enum status = dev->CheckFramebufferStatus(IGLDevice::Framebuffer);
					if(status != IGLDevice::FramebufferComplete) {
						RaiseFBStatusError(status);
					}
					SPLog("Framebuffer Created");
				}catch(...){
					SPLog("Texture creation failed: trying with RGB8");
					useHighPrec = false;
					dev->TexImage2D(IGLDevice::Texture2D,
									0,
									IGLDevice::RGBA8,
									dev->ScreenWidth(),
									dev->ScreenHeight(),
									0,
									IGLDevice::RGBA,
									IGLDevice::UnsignedByte, NULL);
					SPLog("Color Buffer Allocated");
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureMagFilter,
									  IGLDevice::Linear);
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureMinFilter,
									  IGLDevice::Linear);
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureWrapS,
									  IGLDevice::ClampToEdge);
					dev->TexParamater(IGLDevice::Texture2D,
									  IGLDevice::TextureWrapT,
									  IGLDevice::ClampToEdge);
					
					dev->FramebufferTexture2D(IGLDevice::Framebuffer,
											  IGLDevice::ColorAttachment0,
											  IGLDevice::Texture2D,
											  renderColorTexture, 0);
					
					IGLDevice::Enum status = dev->CheckFramebufferStatus(IGLDevice::Framebuffer);
					if(status != IGLDevice::FramebufferComplete) {
						RaiseFBStatusError(status);
					}
					SPLog("Framebuffer Created");
				}
			}
			
			// add render buffer as a registered buffer
			Buffer buf;
			buf.framebuffer = renderFramebuffer;
			buf.texture = renderColorTexture;
			buf.refCount = 0;
			buf.w = device->ScreenWidth();
			buf.h = device->ScreenHeight();
			buf.alpha = false; // actually has alpha, but low-prec (2bit)
			buffers.push_back(buf);
			
			dev->BindFramebuffer(IGLDevice::Framebuffer, 0);
			dev->BindRenderbuffer(IGLDevice::Renderbuffer, 0);
		}
		
		GLFramebufferManager::~GLFramebufferManager(){
			// maybe framebuffers are released automatically when
			// application quits...
		}
		
		void GLFramebufferManager::PrepareSceneRendering() {
			SPADES_MARK_FUNCTION();
			if(useMultisample){
				// ---- multisampled
				device->BindFramebuffer(IGLDevice::Framebuffer,
										multisampledFramebuffer);
				device->Enable(IGLDevice::Multisample, useMultisample);
			}else {
				// ---- single sampled
				device->BindFramebuffer(IGLDevice::Framebuffer,
									 renderFramebuffer);
				
				// calling glDisable(GL_MULTISAMPLE) on non-MSAA FB
				// causes GL_INVALID_FRAMEBUFFER_OPERATION on
				// some video drivers?
			}
			
			device->Enable(IGLDevice::DepthTest, true);
			device->DepthMask(true);
		}
		
		GLColorBuffer GLFramebufferManager::PrepareForWaterRendering(IGLDevice::UInteger tempFb,
																	 IGLDevice::UInteger tempDepthTex) {
			SPADES_MARK_FUNCTION();
			BufferHandle handle;
			
			if(useMultisample){
				handle = BufferHandle(this, 0);
			}else{
				// don't want to the renderBuffer to be returned
				BufferHandle captured = BufferHandle(this, 0);
				handle = CreateBufferHandle(-1, -1, true);
				captured.Release();
			}
			
			
			device->BindFramebuffer(IGLDevice::Framebuffer,
									tempFb);
			device->FramebufferTexture2D(IGLDevice::Framebuffer,
										 IGLDevice::ColorAttachment0,
										 IGLDevice::Texture2D,
										 handle.GetTexture(), 0);
		
			// downsample
			int w = device->ScreenWidth();
			int h = device->ScreenHeight();
			
			if(r_blitFramebuffer){
				if(useMultisample){
					device->BindFramebuffer(IGLDevice::ReadFramebuffer,
											multisampledFramebuffer);
				}else{
					device->BindFramebuffer(IGLDevice::ReadFramebuffer,
											renderFramebuffer);
				}
				device->BindFramebuffer(IGLDevice::DrawFramebuffer,
										tempFb);
				device->BlitFramebuffer(0, 0, w, h,
										0, 0, w, h,
										IGLDevice::ColorBufferBit,
										IGLDevice::Nearest);
				device->BlitFramebuffer(0, 0, w, h,
										0, 0, w, h,
										IGLDevice::DepthBufferBit,
										IGLDevice::Nearest);
				device->BindFramebuffer(IGLDevice::ReadFramebuffer,
										0);
				device->BindFramebuffer(IGLDevice::DrawFramebuffer,
										0);
			}else{
				if(useMultisample){
					device->BindFramebuffer(IGLDevice::Framebuffer,
											multisampledFramebuffer);
				}else{
					device->BindFramebuffer(IGLDevice::Framebuffer,
											renderFramebuffer);
				}
				device->BindTexture(IGLDevice::Texture2D, handle.GetTexture());
				device->CopyTexSubImage2D(IGLDevice::Texture2D,
										  0, 0, 0,
										  0, 0, w, h);
				device->BindTexture(IGLDevice::Texture2D, tempDepthTex);
				device->CopyTexSubImage2D(IGLDevice::Texture2D,
										  0, 0, 0,
										  0, 0, w, h);
			}
			
			
			// restore render framebuffer
			if(useMultisample){
				// ---- multisampled
				device->BindFramebuffer(IGLDevice::Framebuffer,
										multisampledFramebuffer);
			}else {
				// ---- single sampled
				device->BindFramebuffer(IGLDevice::Framebuffer,
										renderFramebuffer);
			}
			
			return handle;
		}
		
		GLFramebufferManager::BufferHandle GLFramebufferManager::StartPostProcessing() {
			SPADES_MARK_FUNCTION();
			if(useMultisample){
				// downsample
				int w = device->ScreenWidth();
				int h = device->ScreenHeight();
				if(r_blitFramebuffer){
					device->BindFramebuffer(IGLDevice::ReadFramebuffer,
											multisampledFramebuffer);
					device->BindFramebuffer(IGLDevice::DrawFramebuffer,
											renderFramebuffer);
					device->BlitFramebuffer(0, 0, w, h,
											0, 0, w, h,
											IGLDevice::ColorBufferBit,
											IGLDevice::Nearest);
					device->BlitFramebuffer(0, 0, w, h,
											0, 0, w, h,
											IGLDevice::DepthBufferBit,
											IGLDevice::Nearest);
					device->BindFramebuffer(IGLDevice::ReadFramebuffer,
											0);
					device->BindFramebuffer(IGLDevice::DrawFramebuffer,
											0);
				}else{
					device->BindFramebuffer(IGLDevice::Framebuffer,
											multisampledFramebuffer);
					device->BindTexture(IGLDevice::Texture2D, renderColorTexture);
					device->CopyTexSubImage2D(IGLDevice::Texture2D,
											  0, 0, 0, 0, 0,
											  w, h);
					device->BindTexture(IGLDevice::Texture2D, renderDepthTexture);
					device->CopyTexSubImage2D(IGLDevice::Texture2D,
											  0, 0, 0, 0, 0,
											  w, h);
				}
			}
			
			device->Enable(IGLDevice::DepthTest, false);
			device->DepthMask(false);
			
			// zero is always renderFramebuffer
			return BufferHandle(this, 0);
		}
		
		void GLFramebufferManager::MakeSureAllBuffersReleased(){
			SPADES_MARK_FUNCTION();
			
			for(size_t i = 0; i < buffers.size(); i++){
				SPAssert(buffers[i].refCount == 0);
			}
		}
		
		GLFramebufferManager::BufferHandle GLFramebufferManager::CreateBufferHandle(int w, int h, bool alpha) {
			SPADES_MARK_FUNCTION();
			
			if(w < 0) w = device->ScreenWidth();
			if(h < 0) h = device->ScreenHeight();
			for(size_t i = 0; i < buffers.size(); i++){
				Buffer& b = buffers[i];
				if(b.refCount > 0)
					continue;
				if(b.w != w || b.h != h)
					continue;
				if(b.alpha != alpha)
					continue;
				return BufferHandle(this, i);
			}
			
			if(buffers.size() > 32){
				SPRaise("Maximum number of framebuffers exceeded");
			}
			
			SPLog("New GLColorBuffer requested (w = %d, h = %d, alpha = %s)",
				  w, h, alpha?"yes":"no");
			
			// no buffer is free!
			IGLDevice::Enum ifmt;
			if(alpha){
				if(r_srgb)
					ifmt = IGLDevice::SRGB8Alpha;
				else
					ifmt = IGLDevice::RGBA8;
			}else{
				if(r_srgb) {
					ifmt = IGLDevice::SRGB8;
				}else{
					if(useHighPrec) {
						ifmt = IGLDevice::RGB10A2;
					}else{
						ifmt = IGLDevice::RGB8;
					}
				}
			}
			
			IGLDevice::UInteger tex = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D,
							 tex);
			device->TexImage2D(IGLDevice::Texture2D,
							0,
							ifmt,
							w,
							h,
							0,
							alpha?IGLDevice::RGBA:IGLDevice::RGB,
							   IGLDevice::UnsignedByte, NULL);
			SPLog("Texture allocated.");
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMagFilter,
							  IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMinFilter,
							  IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapS,
							  IGLDevice::ClampToEdge);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapT,
							  IGLDevice::ClampToEdge);
			
			IGLDevice::UInteger fb = device->GenFramebuffer();
			device->BindFramebuffer(IGLDevice::Framebuffer,
								 fb);
			device->FramebufferTexture2D(IGLDevice::Framebuffer,
									  IGLDevice::ColorAttachment0,
									  IGLDevice::Texture2D,
										 tex, 0);
			SPLog("Framebuffer created.");
			
			device->BindFramebuffer(IGLDevice::Framebuffer, 0);
			
			Buffer buf;
			buf.framebuffer = fb;
			buf.texture = tex;
			buf.refCount = 0;
			buf.w = w;
			buf.h = h;
			buf.alpha = alpha;
			buffers.push_back(buf);
			return BufferHandle(this, buffers.size() - 1);
		}
		
#pragma mark - BufferHandle
		
		GLFramebufferManager::BufferHandle::BufferHandle():
		manager(NULL), bufferIndex(0), valid(false){
			
		}
		
		GLFramebufferManager::BufferHandle::BufferHandle(GLFramebufferManager*m,
														 size_t index):
		manager(m), bufferIndex(index), valid(true){
			SPAssert(bufferIndex < manager->buffers.size());
			Buffer&b = manager->buffers[bufferIndex];
			b.refCount++;
		}
		
		GLFramebufferManager::BufferHandle::BufferHandle(const BufferHandle& other):
		manager(other.manager), bufferIndex(other.bufferIndex), valid(other.valid){
			if(valid){
				Buffer&b = manager->buffers[bufferIndex];
				b.refCount++;
			}
		}
		GLFramebufferManager::BufferHandle::~BufferHandle(){
			Release();
		}
		
		void GLFramebufferManager::BufferHandle::operator=(const BufferHandle& other){
			if(valid){
				manager->buffers[bufferIndex].refCount--;
			}
			manager = other.manager;
			bufferIndex = other.bufferIndex;
			valid = other.valid;
			if(valid){
				manager->buffers[bufferIndex].refCount++;
			}
		}
		
		void GLFramebufferManager::BufferHandle::Release(){
			if(valid){
				Buffer&b = manager->buffers[bufferIndex];
				SPAssert(b.refCount > 0);
				b.refCount--;
				valid = false;
			}
		}
		IGLDevice::UInteger GLFramebufferManager::BufferHandle::GetFramebuffer() {
			SPAssert(valid);
			Buffer&b = manager->buffers[bufferIndex];
			return b.framebuffer;
		}
		IGLDevice::UInteger GLFramebufferManager::BufferHandle::GetTexture() {
			SPAssert(valid);
			Buffer&b = manager->buffers[bufferIndex];
			return b.texture;
		}
		
		int GLFramebufferManager::BufferHandle::GetWidth() {
			SPAssert(valid);
			Buffer&b = manager->buffers[bufferIndex];
			return b.w;
		}
		int GLFramebufferManager::BufferHandle::GetHeight() {
			SPAssert(valid);
			Buffer&b = manager->buffers[bufferIndex];
			return b.h;
		}
		
	}
}


