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
#include "GLSettings.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Exception.h>

namespace spades {
	namespace draw {
		static void RaiseFBStatusError(IGLDevice::Enum status) {
			std::string type;
			switch (status) {
				case IGLDevice::FramebufferComplete: type = "GL_FRAMEBUFFER_COMPLETE"; break;
				case IGLDevice::FramebufferUndefined: type = "GL_FRAMEBUFFER_UNDEFINED"; break;
				case IGLDevice::FramebufferIncompleteAttachment:
					type = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
					break;
				case IGLDevice::FramebufferIncompleteMissingAttachment:
					type = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
					break;
				case IGLDevice::FramebufferIncompleteDrawBuffer:
					type = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
					break;
				case IGLDevice::FramebufferIncompleteReadBuffer:
					type = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
					break;
				case IGLDevice::FramebufferIncompleteMultisample:
					type = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
					break;
				case IGLDevice::FramebufferIncompleteLayerTargets:
					type = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
					break;
				default: // IGLDevice::Enum contains values unrelevant to framebuffer status error
				         // causing static analyzer to say something
					type = "Unknown";
			}
			SPRaise("OpenGL Framebuffer completeness check failed: %s", type.c_str());
		}

		GLFramebufferManager::GLFramebufferManager(IGLDevice &dev, GLSettings &settings,
		                                           int renderWidth, int renderHeight)
		    : device(dev),
		      settings(settings),
		      doingPostProcessing(false),
		      renderWidth(renderWidth),
		      renderHeight(renderHeight) {
			SPADES_MARK_FUNCTION();

			SPLog("Initializing framebuffer manager");

			if (!settings.r_blitFramebuffer && settings.r_multisamples) {
				SPLog("WARNING: Disabling MSAA: no support for MSAA when r_blitFramebuffer = 0");
				settings.r_multisamples = 0;
			}

			useMultisample = (int)settings.r_multisamples > 0;
			useHighPrec = settings.r_highPrec ? 1 : 0;
			useHdr = settings.r_hdr;

			if (useMultisample) {
				SPLog("Multi-sample Antialiasing Enabled");

				// for multisample rendering, use
				// multisample renderbuffer for scene
				// rendering.

				multisampledFramebuffer = dev.GenFramebuffer();
				dev.BindFramebuffer(IGLDevice::Framebuffer, multisampledFramebuffer);

				multisampledDepthRenderbuffer = dev.GenRenderbuffer();
				dev.BindRenderbuffer(IGLDevice::Renderbuffer, multisampledDepthRenderbuffer);
				dev.RenderbufferStorage(IGLDevice::Renderbuffer, (int)settings.r_multisamples,
				                        IGLDevice::DepthComponent24, renderWidth, renderHeight);
				SPLog("MSAA Depth Buffer Allocated");

				dev.FramebufferRenderbuffer(IGLDevice::Framebuffer, IGLDevice::DepthAttachment,
				                            IGLDevice::Renderbuffer, multisampledDepthRenderbuffer);

				multisampledColorRenderbuffer = dev.GenRenderbuffer();
				dev.BindRenderbuffer(IGLDevice::Renderbuffer, multisampledColorRenderbuffer);
				if (settings.r_srgb) {
					SPLog("Creating MSAA Color Buffer with SRGB8_ALPHA");
					useHighPrec = false;
					dev.RenderbufferStorage(IGLDevice::Renderbuffer, (int)settings.r_multisamples,
					                        IGLDevice::SRGB8Alpha, renderWidth, renderHeight);

					SPLog("MSAA Color Buffer Allocated");

					dev.FramebufferRenderbuffer(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
					                            IGLDevice::Renderbuffer,
					                            multisampledColorRenderbuffer);
					IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
					if (status != IGLDevice::FramebufferComplete) {
						RaiseFBStatusError(status);
					}
					fbInternalFormat = IGLDevice::SRGB8Alpha;
					SPLog("MSAA Framebuffer Allocated");
				} else {
					try {
						if (!useHighPrec && !useHdr) {
							SPLog("RGB10A2/HDR disabled");
							SPRaise("jump to catch(...)");
						}
						dev.RenderbufferStorage(IGLDevice::Renderbuffer,
						                        (int)settings.r_multisamples,
						                        useHdr ? IGLDevice::RGBA16F : IGLDevice::RGB10A2,
						                        renderWidth, renderHeight);
						SPLog("MSAA Color Buffer Allocated");

						dev.FramebufferRenderbuffer(
						  IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
						  IGLDevice::Renderbuffer, multisampledColorRenderbuffer);
						IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
						if (status != IGLDevice::FramebufferComplete) {
							RaiseFBStatusError(status);
						}
						fbInternalFormat = useHdr ? IGLDevice::RGBA16F : IGLDevice::RGB10A2;
						SPLog("MSAA Framebuffer Allocated");

					} catch (...) {
						SPLog("Renderbuffer creation failed: trying with RGB8A8");
						useHighPrec = false;
						useHdr = false;
						settings.r_hdr = 0;
						dev.RenderbufferStorage(IGLDevice::Renderbuffer,
						                        (int)settings.r_multisamples, IGLDevice::RGBA8,
						                        renderWidth, renderHeight);

						SPLog("MSAA Color Buffer Allocated");

						dev.FramebufferRenderbuffer(
						  IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
						  IGLDevice::Renderbuffer, multisampledColorRenderbuffer);
						IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
						if (status != IGLDevice::FramebufferComplete) {
							RaiseFBStatusError(status);
						}
						fbInternalFormat = IGLDevice::RGBA8;
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

			renderFramebuffer = dev.GenFramebuffer();
			dev.BindFramebuffer(IGLDevice::Framebuffer, renderFramebuffer);

			renderDepthTexture = dev.GenTexture();
			dev.BindTexture(IGLDevice::Texture2D, renderDepthTexture);
			dev.TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::DepthComponent24, renderWidth,
			               renderHeight, 0, IGLDevice::DepthComponent, IGLDevice::UnsignedInt,
			               NULL);
			SPLog("Depth Buffer Allocated");
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter, IGLDevice::Nearest);
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter, IGLDevice::Nearest);
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS, IGLDevice::ClampToEdge);
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT, IGLDevice::ClampToEdge);

			dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::DepthAttachment,
			                         IGLDevice::Texture2D, renderDepthTexture, 0);

			renderColorTexture = dev.GenTexture();
			dev.BindTexture(IGLDevice::Texture2D, renderColorTexture);
			if (settings.r_srgb) {
				SPLog("Creating Non-MSAA SRGB buffer");
				useHighPrec = false;
				dev.TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::SRGB8Alpha, renderWidth,
				               renderHeight, 0, IGLDevice::RGBA, IGLDevice::UnsignedByte, NULL);
				SPLog("Color Buffer Allocated");
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                 IGLDevice::Linear);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                 IGLDevice::Linear);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                 IGLDevice::ClampToEdge);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                 IGLDevice::ClampToEdge);

				dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
				                         IGLDevice::Texture2D, renderColorTexture, 0);

				IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
				if (status != IGLDevice::FramebufferComplete) {
					RaiseFBStatusError(status);
				}
				fbInternalFormat = IGLDevice::SRGB8Alpha;
				SPLog("Framebuffer Created");
			} else {
				try {
					if (!useHighPrec && !useHdr) {
						SPLog("RGB10A2/HDR disabled");
						SPRaise("jump to catch(...)");
					}
					dev.TexImage2D(
					  IGLDevice::Texture2D, 0, useHdr ? IGLDevice::RGBA16F : IGLDevice::RGB10A2,
					  renderWidth, renderHeight, 0, IGLDevice::RGBA, IGLDevice::UnsignedByte, NULL);
					SPLog("Color Buffer Allocated");
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
					                 IGLDevice::Linear);
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
					                 IGLDevice::Linear);
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
					                 IGLDevice::ClampToEdge);
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
					                 IGLDevice::ClampToEdge);

					dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
					                         IGLDevice::Texture2D, renderColorTexture, 0);

					IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
					if (status != IGLDevice::FramebufferComplete) {
						RaiseFBStatusError(status);
					}
					fbInternalFormat = useHdr ? IGLDevice::RGBA16F : IGLDevice::RGB10A2;
					SPLog("Framebuffer Created");
				} catch (...) {
					SPLog("Texture creation failed: trying with RGB8A8");
					useHighPrec = false;
					useHdr = false;
					settings.r_hdr = 0;
					dev.TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA8, renderWidth,
					               renderHeight, 0, IGLDevice::RGBA, IGLDevice::UnsignedByte,
					               NULL);
					SPLog("Color Buffer Allocated");
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
					                 IGLDevice::Linear);
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
					                 IGLDevice::Linear);
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
					                 IGLDevice::ClampToEdge);
					dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
					                 IGLDevice::ClampToEdge);

					dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
					                         IGLDevice::Texture2D, renderColorTexture, 0);

					IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
					if (status != IGLDevice::FramebufferComplete) {
						RaiseFBStatusError(status);
					}
					fbInternalFormat = IGLDevice::RGBA8;
					SPLog("Framebuffer Created");
				}
			}

			if ((int)settings.r_water >= 2) {
				SPLog("Creating Mirror framebuffer");
				mirrorFramebuffer = dev.GenFramebuffer();
				dev.BindFramebuffer(IGLDevice::Framebuffer, mirrorFramebuffer);

				mirrorColorTexture = dev.GenTexture();
				dev.BindTexture(IGLDevice::Texture2D, mirrorColorTexture);
				SPLog("Creating Mirror texture");
				dev.TexImage2D(IGLDevice::Texture2D, 0, fbInternalFormat, renderWidth,
				               renderHeight, 0, IGLDevice::RGBA, IGLDevice::UnsignedByte, NULL);

				SPLog("Color Buffer Allocated");
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                 IGLDevice::Linear);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                 IGLDevice::Linear);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                 IGLDevice::ClampToEdge);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                 IGLDevice::ClampToEdge);

				dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
				                         IGLDevice::Texture2D, mirrorColorTexture, 0);

				SPLog("Creating Mirror depth texture");
				mirrorDepthTexture = dev.GenTexture();
				dev.BindTexture(IGLDevice::Texture2D, mirrorDepthTexture);
				dev.TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::DepthComponent24, renderWidth,
				               renderHeight, 0, IGLDevice::DepthComponent, IGLDevice::UnsignedInt,
				               NULL);

				SPLog("Depth Buffer Allocated");
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                 IGLDevice::Nearest);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                 IGLDevice::Nearest);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                 IGLDevice::ClampToEdge);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                 IGLDevice::ClampToEdge);

				dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::DepthAttachment,
				                         IGLDevice::Texture2D, mirrorDepthTexture, 0);

				IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
				if (status != IGLDevice::FramebufferComplete) {
					RaiseFBStatusError(status);
				}
				SPLog("Mirror Framebuffer Created");
			} // (int)r_water >= 2

			renderFramebufferWithoutDepth = dev.GenFramebuffer();
			dev.BindFramebuffer(IGLDevice::Framebuffer, renderFramebufferWithoutDepth);
			dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
			                         IGLDevice::Texture2D, renderColorTexture, 0);

			// add render buffer as a registered buffer
			Buffer buf;
			buf.framebuffer = renderFramebufferWithoutDepth;
			buf.texture = renderColorTexture;
			buf.refCount = 0;
			buf.w = renderWidth;
			buf.h = renderHeight;
			buf.internalFormat = fbInternalFormat;
			buffers.push_back(buf);

			dev.BindFramebuffer(IGLDevice::Framebuffer, 0);
			dev.BindRenderbuffer(IGLDevice::Renderbuffer, 0);
		}

		GLFramebufferManager::~GLFramebufferManager() {
			if (multisampledFramebuffer) {
				device.DeleteFramebuffer(multisampledFramebuffer);
			}
			if (multisampledColorRenderbuffer) {
				device.DeleteRenderbuffer(multisampledColorRenderbuffer);
			}
			if (multisampledDepthRenderbuffer) {
				device.DeleteRenderbuffer(multisampledDepthRenderbuffer);
			}
			if (renderFramebuffer) {
				device.DeleteFramebuffer(renderFramebuffer);
			}
			if (renderColorTexture) {
				device.DeleteTexture(renderColorTexture);
			}
			if (renderDepthTexture) {
				device.DeleteTexture(renderDepthTexture);
			}
			if (mirrorFramebuffer) {
				device.DeleteFramebuffer(mirrorFramebuffer);
			}
			if (mirrorColorTexture) {
				device.DeleteTexture(mirrorColorTexture);
			}
			if (mirrorDepthTexture) {
				device.DeleteTexture(mirrorDepthTexture);
			}
			for (const Buffer &buffer : buffers) {
				device.DeleteFramebuffer(buffer.framebuffer);
				device.DeleteTexture(buffer.texture);
			}
			buffers.clear();
		}

		void GLFramebufferManager::PrepareSceneRendering() {
			SPADES_MARK_FUNCTION();
			if (useMultisample) {
				// ---- multisampled
				device.BindFramebuffer(IGLDevice::Framebuffer, multisampledFramebuffer);
				device.Enable(IGLDevice::Multisample, useMultisample);
			} else {
				// ---- single sampled
				device.BindFramebuffer(IGLDevice::Framebuffer, renderFramebuffer);

				// calling glDisable(GL_MULTISAMPLE) on non-MSAA FB
				// causes GL_INVALID_FRAMEBUFFER_OPERATION on
				// some video drivers?
			}

			doingPostProcessing = false;

			device.Enable(IGLDevice::DepthTest, true);
			device.DepthMask(true);
			device.Viewport(0, 0, renderWidth, renderHeight);
		}

		GLColorBuffer
		GLFramebufferManager::PrepareForWaterRendering(IGLDevice::UInteger tempFb,
		                                               IGLDevice::UInteger tempDepthTex) {
			SPADES_MARK_FUNCTION();
			BufferHandle handle;

			if (useMultisample) {
				handle = BufferHandle(this, 0);
			} else {
				// don't want to the renderBuffer to be returned
				BufferHandle captured = BufferHandle(this, 0);
				handle = CreateBufferHandle(-1, -1, true);
				captured.Release();
			}

			device.BindFramebuffer(IGLDevice::Framebuffer, tempFb);
			device.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
			                            IGLDevice::Texture2D, handle.GetTexture(), 0);

			// downsample
			int w = renderWidth;
			int h = renderHeight;

			if (settings.r_blitFramebuffer) {
				if (useMultisample) {
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, multisampledFramebuffer);
				} else {
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, renderFramebuffer);
				}
				device.BindFramebuffer(IGLDevice::DrawFramebuffer, tempFb);
				device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::ColorBufferBit,
				                       IGLDevice::Nearest);
				device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::DepthBufferBit,
				                       IGLDevice::Nearest);
				device.BindFramebuffer(IGLDevice::ReadFramebuffer, 0);
				device.BindFramebuffer(IGLDevice::DrawFramebuffer, 0);
			} else {
				if (useMultisample) {
					device.BindFramebuffer(IGLDevice::Framebuffer, multisampledFramebuffer);
				} else {
					device.BindFramebuffer(IGLDevice::Framebuffer, renderFramebuffer);
				}
				device.BindTexture(IGLDevice::Texture2D, handle.GetTexture());
				device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
				device.BindTexture(IGLDevice::Texture2D, tempDepthTex);
				device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
			}

			// restore render framebuffer
			if (useMultisample) {
				// ---- multisampled
				device.BindFramebuffer(IGLDevice::Framebuffer, multisampledFramebuffer);
			} else {
				// ---- single sampled
				device.BindFramebuffer(IGLDevice::Framebuffer, renderFramebuffer);
			}

			return handle;
		}

		void GLFramebufferManager::ClearMirrorTexture(spades::Vector3 bgCol) {
			device.BindFramebuffer(IGLDevice::Framebuffer, mirrorFramebuffer);
			device.Viewport(0, 0, renderWidth, renderHeight);
			device.ClearColor(bgCol.x, bgCol.y, bgCol.z, 1.f);
			device.Clear((IGLDevice::Enum)(IGLDevice::ColorBufferBit | IGLDevice::DepthBufferBit));

			// restore framebuffer
			if (useMultisample) {
				// ---- multisampled
				device.BindFramebuffer(IGLDevice::Framebuffer, multisampledFramebuffer);
			} else {
				// ---- single sampled
				device.BindFramebuffer(IGLDevice::Framebuffer, renderFramebuffer);
			}
		}

		void GLFramebufferManager::CopyToMirrorTexture(IGLDevice::UInteger fb) {
			SPADES_MARK_FUNCTION();
			int w = renderWidth;
			int h = renderHeight;
			if (fb == 0) {
				fb = useMultisample ? multisampledFramebuffer : renderFramebuffer;
			}

			bool needsDepth = (int)settings.r_water >= 3;

			if (useMultisample) {
				// downsample
				if (settings.r_blitFramebuffer) {
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, fb);
					device.BindFramebuffer(IGLDevice::DrawFramebuffer, mirrorFramebuffer);
					device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::ColorBufferBit,
					                       IGLDevice::Nearest);
					if (needsDepth) {
						device.BindFramebuffer(IGLDevice::ReadFramebuffer, renderFramebuffer);
						device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::DepthBufferBit,
						                       IGLDevice::Nearest);
					}
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, 0);
					device.BindFramebuffer(IGLDevice::DrawFramebuffer, 0);
				} else {
					device.BindFramebuffer(IGLDevice::Framebuffer, fb);
					device.BindTexture(IGLDevice::Texture2D, mirrorColorTexture);
					device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
					if (needsDepth) {
						device.BindTexture(IGLDevice::Texture2D, mirrorDepthTexture);
						device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
					}
				}
			} else {
				// copy
				if (settings.r_blitFramebuffer) {
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, fb);
					device.BindFramebuffer(IGLDevice::DrawFramebuffer, mirrorFramebuffer);
					device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::ColorBufferBit,
					                       IGLDevice::Nearest);
					if (needsDepth) {
						device.BindFramebuffer(IGLDevice::ReadFramebuffer, renderFramebuffer);
						device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::DepthBufferBit,
						                       IGLDevice::Nearest);
					}
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, 0);
					device.BindFramebuffer(IGLDevice::DrawFramebuffer, 0);
				} else {
					device.BindFramebuffer(IGLDevice::Framebuffer, fb);
					device.BindTexture(IGLDevice::Texture2D, mirrorColorTexture);
					device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
					if (needsDepth) {
						device.BindTexture(IGLDevice::Texture2D, mirrorDepthTexture);
						device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
					}
				}
			}

			device.BindTexture(IGLDevice::Texture2D, mirrorColorTexture);
			// device.GenerateMipmap(IGLDevice::Texture2D);

			// restore framebuffer
			if (useMultisample) {
				// ---- multisampled
				device.BindFramebuffer(IGLDevice::Framebuffer, multisampledFramebuffer);
			} else {
				// ---- single sampled
				device.BindFramebuffer(IGLDevice::Framebuffer, renderFramebuffer);
			}

			device.Enable(IGLDevice::DepthTest, true);
			device.DepthMask(true);
		}

		GLFramebufferManager::BufferHandle GLFramebufferManager::StartPostProcessing() {
			SPADES_MARK_FUNCTION();

			doingPostProcessing = true;

			if (useMultisample) {
				// downsample
				int w = renderWidth;
				int h = renderHeight;
				if (settings.r_blitFramebuffer) {
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, multisampledFramebuffer);
					device.BindFramebuffer(IGLDevice::DrawFramebuffer, renderFramebuffer);
					device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::ColorBufferBit,
					                       IGLDevice::Nearest);
					device.BlitFramebuffer(0, 0, w, h, 0, 0, w, h, IGLDevice::DepthBufferBit,
					                       IGLDevice::Nearest);
					device.BindFramebuffer(IGLDevice::ReadFramebuffer, 0);
					device.BindFramebuffer(IGLDevice::DrawFramebuffer, 0);
				} else {
					device.BindFramebuffer(IGLDevice::Framebuffer, multisampledFramebuffer);
					device.BindTexture(IGLDevice::Texture2D, renderColorTexture);
					device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
					device.BindTexture(IGLDevice::Texture2D, renderDepthTexture);
					device.CopyTexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 0, 0, w, h);
				}
			}

			device.Enable(IGLDevice::DepthTest, false);
			device.DepthMask(false);

			// zero is always renderFramebuffer
			return BufferHandle(this, 0);
		}

		void GLFramebufferManager::MakeSureAllBuffersReleased() {
			SPADES_MARK_FUNCTION();

			for (size_t i = 0; i < buffers.size(); i++) {
				SPAssert(buffers[i].refCount == 0);
			}
		}

		GLFramebufferManager::BufferHandle GLFramebufferManager::CreateBufferHandle(int w, int h,
		                                                                            bool alpha) {
			IGLDevice::Enum ifmt;
			if (alpha) {
				if (settings.r_srgb)
					ifmt = IGLDevice::SRGB8Alpha;
				else
					ifmt = IGLDevice::RGBA8;
			} else {
				ifmt = fbInternalFormat;
			}
			return CreateBufferHandle(w, h, ifmt);
		}
		GLFramebufferManager::BufferHandle
		GLFramebufferManager::CreateBufferHandle(int w, int h, IGLDevice::Enum iFormat) {
			SPADES_MARK_FUNCTION();

			if (w < 0)
				w = renderWidth;
			if (h < 0)
				h = renderHeight;

			// During the main rendering pass the first buffer is allocated to the render target
			// and cannot be allocated for pre/postprocessing pass
			for (size_t i = doingPostProcessing ? 0 : 1; i < buffers.size(); i++) {
				Buffer &b = buffers[i];
				if (b.refCount > 0)
					continue;
				if (b.w != w || b.h != h)
					continue;
				if (b.internalFormat != iFormat)
					continue;
				return BufferHandle(this, i);
			}

			if (buffers.size() > 128) {
				SPRaise("Maximum number of framebuffers exceeded");
			}

			SPLog("New GLColorBuffer requested (w = %d, h = %d, ifmt = 0x%04x)", w, h,
			      (int)iFormat);

			// no buffer is free!
			IGLDevice::Enum ifmt = iFormat;

			IGLDevice::UInteger tex = device.GenTexture();
			device.BindTexture(IGLDevice::Texture2D, tex);
			device.TexImage2D(IGLDevice::Texture2D, 0, ifmt, w, h, 0, IGLDevice::Red,
			                  IGLDevice::UnsignedByte, NULL);
			SPLog("Texture allocated.");
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                    IGLDevice::Linear);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                    IGLDevice::Linear);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
			                    IGLDevice::ClampToEdge);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
			                    IGLDevice::ClampToEdge);

			IGLDevice::UInteger fb = device.GenFramebuffer();
			device.BindFramebuffer(IGLDevice::Framebuffer, fb);
			device.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
			                            IGLDevice::Texture2D, tex, 0);
			SPLog("Framebuffer created.");

			device.BindFramebuffer(IGLDevice::Framebuffer, 0);

			Buffer buf;
			buf.framebuffer = fb;
			buf.texture = tex;
			buf.refCount = 0;
			buf.w = w;
			buf.h = h;
			buf.internalFormat = ifmt;
			buffers.push_back(buf);
			return BufferHandle(this, buffers.size() - 1);
		}

#pragma mark - BufferHandle

		GLFramebufferManager::BufferHandle::BufferHandle()
		    : manager(NULL), bufferIndex(0), valid(false) {}

		GLFramebufferManager::BufferHandle::BufferHandle(GLFramebufferManager *m, size_t index)
		    : manager(m), bufferIndex(index), valid(true) {
			SPAssert(bufferIndex < manager->buffers.size());
			Buffer &b = manager->buffers[bufferIndex];
			b.refCount++;
		}

		GLFramebufferManager::BufferHandle::BufferHandle(const BufferHandle &other)
		    : manager(other.manager), bufferIndex(other.bufferIndex), valid(other.valid) {
			if (valid) {
				Buffer &b = manager->buffers[bufferIndex];
				b.refCount++;
			}
		}
		GLFramebufferManager::BufferHandle::~BufferHandle() { Release(); }

		void GLFramebufferManager::BufferHandle::operator=(const BufferHandle &other) {
			if (valid) {
				manager->buffers[bufferIndex].refCount--;
			}
			manager = other.manager;
			bufferIndex = other.bufferIndex;
			valid = other.valid;
			if (valid) {
				manager->buffers[bufferIndex].refCount++;
			}
		}

		void GLFramebufferManager::BufferHandle::Release() {
			if (valid) {
				Buffer &b = manager->buffers[bufferIndex];
				SPAssert(b.refCount > 0);
				b.refCount--;
				valid = false;
			}
		}
		IGLDevice::UInteger GLFramebufferManager::BufferHandle::GetFramebuffer() {
			SPAssert(valid);
			Buffer &b = manager->buffers[bufferIndex];
			return b.framebuffer;
		}
		IGLDevice::UInteger GLFramebufferManager::BufferHandle::GetTexture() {
			SPAssert(valid);
			Buffer &b = manager->buffers[bufferIndex];
			return b.texture;
		}

		int GLFramebufferManager::BufferHandle::GetWidth() {
			SPAssert(valid);
			Buffer &b = manager->buffers[bufferIndex];
			return b.w;
		}
		int GLFramebufferManager::BufferHandle::GetHeight() {
			SPAssert(valid);
			Buffer &b = manager->buffers[bufferIndex];
			return b.h;
		}
		IGLDevice::Enum GLFramebufferManager::BufferHandle::GetInternalFormat() {
			SPAssert(valid);
			Buffer &b = manager->buffers[bufferIndex];
			return b.internalFormat;
		}
	} // namespace draw
} // namespace spades
