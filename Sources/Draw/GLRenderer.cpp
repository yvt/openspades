//
//  GLRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLRenderer.h"
#include "IGLDevice.h"
#include "GLProgramManager.h"
#include "GLProgramUniform.h"
#include "GLProgramAttribute.h"
#include "GLImageManager.h"
#include "GLMapRenderer.h"
#include "GLImage.h"
#include "GLModelManager.h"
#include "GLModel.h"
#include "../Core/Exception.h"
#include "../Core/Debug.h"
#include "GLVoxelModel.h"
#include "GLImageRenderer.h"
#include "../Client/GameMap.h"
#include "GLSpriteRenderer.h"
#include "GLSoftSpriteRenderer.h"
#include "GLFlatMapRenderer.h"
#include "GLFramebufferManager.h"
#include "GLBloomFilter.h"
#include "GLLensFilter.h"
#include "../Core/Settings.h"
#include "GLMapShadowRenderer.h"
#include "../Core/Bitmap.h"
#include "GLModelRenderer.h"
#include "GLShadowMapShader.h"
#include "IGLShadowMapRenderer.h"
#include "GLOptimizedVoxelModel.h"
#include "GLWaterRenderer.h"
#include "GLAmbientShadowRenderer.h"
#include "GLRadiosityRenderer.h"
#include "GLFogFilter.h"
#include "GLLensFlareFilter.h"
#include "GLFXAAFilter.h"
#include "../Core/Stopwatch.h"
#include <stdarg.h>
#include <stdlib.h>
#include "GLProfiler.h"

SPADES_SETTING(r_water, "1");
SPADES_SETTING(r_bloom, "1");
SPADES_SETTING(r_lens, "1");
SPADES_SETTING(r_lensFlare, "1");
SPADES_SETTING(r_softParticles, "1");
SPADES_SETTING(r_cameraBlur, "1");
SPADES_SETTING(r_dlights, "1");
SPADES_SETTING(r_optimizedVoxelModel, "1");
SPADES_SETTING(r_radiosity, "0");
SPADES_SETTING(r_fogShadow, "0");
SPADES_SETTING(r_fxaa, "1");
SPADES_SETTING(r_srgb, "1");

SPADES_SETTING(r_debugTiming, "0");

namespace spades {
	namespace draw {
		
				
		GLRenderer::GLRenderer(IGLDevice *_device):
		device(_device),
		cameraBlur(this){
			SPADES_MARK_FUNCTION();
			
			SPLog("GLRenderer initializing");
			
			fbManager = new GLFramebufferManager(_device);
			shadowMapRenderer = GLShadowMapShader::CreateShadowMapRenderer(this);
			programManager = new GLProgramManager(_device, shadowMapRenderer);
			imageManager = new GLImageManager(_device);
			modelManager = new GLModelManager(this);
			mapShadowRenderer = NULL;
			mapRenderer = NULL;
			imageRenderer = new GLImageRenderer(this);
			flatMapRenderer = NULL;
			if(r_softParticles)
				spriteRenderer = new GLSoftSpriteRenderer(this);
			else
				spriteRenderer = new GLSpriteRenderer(this);
			modelRenderer = new GLModelRenderer(this);
			waterRenderer = NULL;
			ambientShadowRenderer = NULL;
			radiosityRenderer = NULL;
			lastTime = 0;
			
			SPLog("GLRenderer initialized");
		}
		
		GLRenderer::~GLRenderer() {
			SPADES_MARK_FUNCTION();
			
			// FIXME: remove itself from map's listener
			
			SPLog("GLRender finalizing");
			if(radiosityRenderer)
				delete radiosityRenderer;
			if(ambientShadowRenderer)
				delete ambientShadowRenderer;
			if(flatMapRenderer)
				delete flatMapRenderer;
			if(mapShadowRenderer)
				delete mapShadowRenderer;
			if(mapRenderer)
				delete mapRenderer;
			if(waterRenderer)
				delete waterRenderer;
			if(ambientShadowRenderer)
				delete ambientShadowRenderer;
			if(shadowMapRenderer)
				delete shadowMapRenderer;
			delete waterRenderer;
			delete modelRenderer;
			delete spriteRenderer;
			delete imageRenderer;
			delete modelManager;
			delete programManager;
			delete imageManager;
			delete fbManager;
			SPLog("GLRenderer finalized");
		}
		
		client::IImage *GLRenderer::RegisterImage(const char *filename) {
			SPADES_MARK_FUNCTION();
			return imageManager->RegisterImage(filename);
		}
		
		client::IModel *GLRenderer::RegisterModel(const char *filename){
			SPADES_MARK_FUNCTION();
			return modelManager->RegisterModel(filename);
		}
		
		client::IImage *GLRenderer::CreateImage(spades::Bitmap *bmp) {
			SPADES_MARK_FUNCTION();
			return GLImage::FromBitmap(bmp,
									   device);
		}
		
		client::IModel *GLRenderer::CreateModel(spades::VoxelModel *model) {
			SPADES_MARK_FUNCTION();
			return new GLVoxelModel(model, this);
		}
		
		client::IModel *GLRenderer::CreateModelOptimized(spades::VoxelModel *model) {
			SPADES_MARK_FUNCTION();
			if(r_optimizedVoxelModel){
				return new GLOptimizedVoxelModel(model, this);
			}else{
				return new GLVoxelModel(model, this);
			}
		}
		
		void GLRenderer::SetGameMap(client::GameMap *mp){
			SPADES_MARK_FUNCTION();
			
			SPLog("New map loaded; freeing old renderers...");
			if(radiosityRenderer)
				delete radiosityRenderer;
			if(mapRenderer)
				delete mapRenderer;
			if(flatMapRenderer)
				delete flatMapRenderer;
			if(mapShadowRenderer)
				delete mapShadowRenderer;
			if(waterRenderer)
				delete waterRenderer;
			if(ambientShadowRenderer)
				delete ambientShadowRenderer;
			radiosityRenderer = NULL;
			if(mp){
				SPLog("Creating new renderers...");
				
				SPLog("Creating Terrain Shadow Map Renderer");
				mapShadowRenderer = new GLMapShadowRenderer(this,mp);
				SPLog("Creating TerrainRenderer");
				mapRenderer = new GLMapRenderer(mp, this);
				SPLog("Creating Minimap Renderer");
				flatMapRenderer = new GLFlatMapRenderer(this, mp);
				SPLog("Creating Water Renderer");
				waterRenderer = new GLWaterRenderer(this, mp);
				
				if(r_radiosity){
					SPLog("Creating Ray-traced Ambient Occlusion Renderer");
					ambientShadowRenderer = new GLAmbientShadowRenderer(this, mp);
					SPLog("Creating Relective Shadow Maps Renderer");
					radiosityRenderer = new GLRadiosityRenderer(this, mp);
				}else{
					SPLog("Radiosity is disabled");
					
					ambientShadowRenderer = NULL;
				}
				
				mp->SetListener(this);
				SPLog("Created");
			}else{
				SPLog("No map loaded");
				mapShadowRenderer = NULL;
				mapRenderer = NULL;
				flatMapRenderer = NULL;
				waterRenderer = NULL;
				ambientShadowRenderer = NULL;
			}
		}
		
		float GLRenderer::ScreenWidth() {
			return device->ScreenWidth();
		}
		
		float GLRenderer::ScreenHeight() {
			return device->ScreenHeight();
		}
		
		Vector3 GLRenderer::GetFogColorForSolidPass() {
			if(r_fogShadow && mapShadowRenderer){
				return MakeVector3(0, 0, 0);
			}else{
				return GetFogColor();
			}
		}

#pragma mark - Resource Manager
		
		GLProgram *GLRenderer::RegisterProgram(const std::string &name){
			return programManager->RegisterProgram(name);
		}
		
		GLShader *GLRenderer::RegisterShader(const std::string &name){
			return programManager->RegisterShader(name);
		}
		
#pragma mark - Scene Intiializer
		
		void GLRenderer::BuildProjectionMatrix() {
			SPADES_MARK_FUNCTION();
			
			float near = sceneDef.zNear;
			float far = sceneDef.zFar;
			float t = near * tanf(sceneDef.fovY * .5f);
			float r = near * tanf(sceneDef.fovX * .5f);
			float a = r * 2.f, b = t * 2.f, c = far - near;
			Matrix4 mat;
			mat.m[0] = near * 2.f / a;
			mat.m[1] = 0.f;
			mat.m[2] = 0.f;
			mat.m[3] = 0.f;
			mat.m[4] = 0.f;
			mat.m[5] = near * 2.f / b;
			mat.m[6] = 0.f;
			mat.m[7] = 0.f;
			mat.m[8] = 0.f;
			mat.m[9] = 0.f;
			mat.m[10] = -(far + near) / c;
			mat.m[11] = -1.f;
			mat.m[12] = 0.f;
			mat.m[13] = 0.f;
			mat.m[14] = -(far * near * 2.f) / c;
			mat.m[15] = 0.f;
			projectionMatrix = mat;
		}
		
		void GLRenderer::BuildView() {
			SPADES_MARK_FUNCTION();
			
			Matrix4 mat = Matrix4::Identity();
			mat.m[0] = sceneDef.viewAxis[0].x;
			mat.m[4] = sceneDef.viewAxis[0].y;
			mat.m[8] = sceneDef.viewAxis[0].z;
			mat.m[1] = sceneDef.viewAxis[1].x;
			mat.m[5] = sceneDef.viewAxis[1].y;
			mat.m[9] = sceneDef.viewAxis[1].z;
			mat.m[2] = -sceneDef.viewAxis[2].x;
			mat.m[6] = -sceneDef.viewAxis[2].y;
			mat.m[10] = -sceneDef.viewAxis[2].z;
			
			Vector4 v = mat * sceneDef.viewOrigin;
			mat.m[12] = -v.x;
			mat.m[13] = -v.y;
			mat.m[14] = -v.z;
			
			viewMatrix = mat;
		}
		
		void GLRenderer::BuildFrustrum() {
			// far/near
			frustrum[0] = Plane3::PlaneWithPointOnPlane(sceneDef.viewOrigin,
														sceneDef.viewAxis[2]);
			frustrum[1] = frustrum[0].Flipped();
			frustrum[0].w -= sceneDef.zNear;
			frustrum[1].w += sceneDef.zFar;
			
			float xCos = cosf(sceneDef.fovX * .5f);
			float xSin = sinf(sceneDef.fovX * .5f);
			float yCos = cosf(sceneDef.fovY * .5f);
			float ySin = sinf(sceneDef.fovY * .5f);
			
			frustrum[2] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * xSin - sceneDef.viewAxis[0] * xCos);
			frustrum[3] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * xSin + sceneDef.viewAxis[0] * xCos);
			frustrum[4] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * ySin - sceneDef.viewAxis[1] * yCos);
			frustrum[5] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * ySin + sceneDef.viewAxis[1] * yCos);
			
		}
		
		void GLRenderer::StartScene(const client::SceneDefinition &def) {
			SPADES_MARK_FUNCTION();
			
			sceneDef = def;
			fbManager->PrepareSceneRendering();
			
			// clear scene objects
			debugLines.clear();
			spriteRenderer->Clear();
			lights.clear();
			
			device->DepthMask(true);
			
			BuildProjectionMatrix();
			BuildView();
			BuildFrustrum();
			
			projectionViewMatrix = projectionMatrix * viewMatrix;
			
			device->ClearDepth(1.f);
			Vector3 bgCol = GetFogColorForSolidPass();
			device->ClearColor(bgCol.x, bgCol.y, bgCol.z, 1.f);
			device->Clear((IGLDevice::Enum)(IGLDevice::ColorBufferBit | IGLDevice::DepthBufferBit));
			device->DepthRange(0.f, 1.f);
			device->Enable(IGLDevice::Blend, false);
			//device-(IGLDevice::Front);
			
			device->Enable(IGLDevice::DepthTest, true);
			device->Enable(IGLDevice::Texture2D, true);
			
			if(r_srgb)
				device->Enable(IGLDevice::FramebufferSRGB, false);
			
		}
		
#pragma mark - Add Scene Objects
		
		void GLRenderer::RenderModel(client::IModel *model,
									 const client::ModelRenderParam & param){
			SPADES_MARK_FUNCTION();
			
			GLModel *m = dynamic_cast<GLModel *>(model);
			if(!m){
				SPInvalidArgument("model");
			}
			
			// TODO: early frustrum cull?
			
			modelRenderer->AddModel(m, param);
			
		}
		
		void GLRenderer::AddLight(const client::DynamicLightParam &light) {
			if(!r_dlights)
				return;
			if(!SphereFrustrumCull(light.origin, light.radius))
				return;
			lights.push_back(GLDynamicLight(light));
		}
		
		void GLRenderer::AddDebugLine(spades::Vector3 a,
									  spades::Vector3 b,
									  spades::Vector4 color) {
			DebugLine line = {a, b, color};
			debugLines.push_back(line);
		}
		
		void GLRenderer::AddSprite(client::IImage *img,
								   spades::Vector3 center,
								   float radius,
								   float rotation){
			SPADES_MARK_FUNCTION_DEBUG();
			GLImage *im = dynamic_cast<GLImage *>(img);
			if(!im)
				SPInvalidArgument("im");
			
			if(!SphereFrustrumCull(center, radius * 1.5f))
				return;
			
			spriteRenderer->Add(im, center, radius, rotation,
								drawColor);
		}
		
#pragma mark - Scene Finalizer
		
		struct DebugLineVertex {
			float x, y, z;
			float r, g, b, a;
			static DebugLineVertex Create(Vector3 v,
										  Vector4 col){
				DebugLineVertex vv = {v.x, v.y, v.z, col.x, col.y, col.z, col.w};
				return vv;
			}
		};
		void GLRenderer::RenderDebugLines() {
			SPADES_MARK_FUNCTION();
			if(debugLines.empty())
				return;
			
			// build vertices
			std::vector<DebugLineVertex> vertices;
			vertices.resize(debugLines.size() * 2);
			
			for(size_t i = 0, j = 0; i < debugLines.size();
				i++){
				const DebugLine& line = debugLines[i];
				vertices[j++] = DebugLineVertex::Create(line.v1, line.color);
				vertices[j++] = DebugLineVertex::Create(line.v2, line.color);
			}
			
			GLProgram *program = RegisterProgram("Shaders/Basic.program");
			program->Use();
			
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			
			positionAttribute(program);
			colorAttribute(program);
			
			device->VertexAttribPointer(positionAttribute(),
										3, IGLDevice::FloatType,
										false, sizeof(DebugLineVertex),
										vertices.data());
			device->VertexAttribPointer(colorAttribute(),
										4, IGLDevice::FloatType,
										false, sizeof(DebugLineVertex),
										(const char*)vertices.data() + sizeof(float) * 3);
			
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(colorAttribute(), true);
			
			static GLProgramUniform projectionViewMatrix("projectionViewMatrix");
			projectionViewMatrix(program);
			projectionViewMatrix.SetValue(GetProjectionViewMatrix());
			
			device->DrawArrays(IGLDevice::Lines,
							   0,
							   vertices.size());
			
			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(colorAttribute(), false);
			
		}
		
		void GLRenderer::EndScene() {
			SPADES_MARK_FUNCTION();
			
			GLProfiler rootProfiler(device, "EndScene");
			
			float dt = (float)(sceneDef.time - lastTime) / 1000.f;
			if(dt > .1f) dt = .1f;
			if(dt < 0.f) dt = 0.f;
			
			{
				GLProfiler profiler(device, "Uploading Software Rendered Stuff");
				if(mapShadowRenderer)
					mapShadowRenderer->Update();
				if(ambientShadowRenderer){
					ambientShadowRenderer->Update();
				}
				if(radiosityRenderer){
					radiosityRenderer->Update();
				}
			}
				
			// build shadowmap
			{
				GLProfiler profiler(device, "Shadow Map Pass");
				device->Enable(IGLDevice::DepthTest, true);
				device->DepthFunc(IGLDevice::Less);
				if(shadowMapRenderer)
					shadowMapRenderer->Render();
			}
			
			// draw opaque objects, and do dynamic lighting
			{
				GLProfiler profiler(device, "Sunlight Pass");
				
				device->DepthFunc(IGLDevice::Less);
				if(!sceneDef.skipWorld && mapRenderer){
					mapRenderer->Prerender();
				}
				modelRenderer->Prerender();
				
				if(!sceneDef.skipWorld && mapRenderer){
					mapRenderer->RenderSunlightPass();
				}
				modelRenderer->RenderSunlightPass();
			}
			
			{
				GLProfiler profiler(device, "Dynamic Light Pass [%d light(s)]", (int)lights.size());
				
				device->Enable(IGLDevice::Blend, true);
				device->Enable(IGLDevice::DepthTest, true);
				device->DepthFunc(IGLDevice::Equal);
				device->BlendFunc(IGLDevice::SrcAlpha,
								  IGLDevice::One);
				
				if(!sceneDef.skipWorld && mapRenderer){
					mapRenderer->RenderDynamicLightPass(lights);
				}
				modelRenderer->RenderDynamicLightPass(lights);
			}
			
			{
				GLProfiler profiler(device, "Debug Line");
				device->Enable(IGLDevice::Blend, false);
				device->Enable(IGLDevice::DepthTest, true);
				device->DepthFunc(IGLDevice::Less);
				RenderDebugLines();
			}
			
			device->Enable(IGLDevice::CullFace, false);
			if(r_water && waterRenderer){
				GLProfiler profiler(device, "Water");
				waterRenderer->Update(dt);
				waterRenderer->Render();
			}
			
			device->Enable(IGLDevice::Blend, true);
			
			if(r_srgb)
				device->Enable(IGLDevice::FramebufferSRGB, true);
			
			device->DepthMask(false);
			if(!r_softParticles){ // softparticle is a part of postprocess
				GLProfiler profiler(device, "Particle");
				device->BlendFunc(IGLDevice::One,
								  IGLDevice::OneMinusSrcAlpha);
				spriteRenderer->Render();
			}
			
			
			
			device->Enable(IGLDevice::DepthTest, false);
			
			GLFramebufferManager::BufferHandle handle;
			{
				GLProfiler profiler(device, "Post-process");
				
				// now process the non-multisampled buffer.
				// depth buffer is also can be read
				{
					GLProfiler profiler(device, "Preparation");
					handle = fbManager->StartPostProcessing();
				}
				if(r_fogShadow && mapShadowRenderer &&
				   fogColor.GetPoweredLength() > .000001f) {
					GLProfiler profiler(device, "Volumetric Fog");
					GLFogFilter fogfilter(this);
					handle = fogfilter.Filter(handle);
				}
				device->BindFramebuffer(IGLDevice::Framebuffer, handle.GetFramebuffer());
				
				if(r_softParticles) {// softparticle is a part of postprocess
					GLProfiler profiler(device, "Soft Particle");
					device->BlendFunc(IGLDevice::One,
									  IGLDevice::OneMinusSrcAlpha);
					spriteRenderer->Render();
				}
				
				
				device->BlendFunc(IGLDevice::SrcAlpha,
								  IGLDevice::OneMinusSrcAlpha);
				
				if(r_cameraBlur && !sceneDef.denyCameraBlur){
					GLProfiler profiler(device, "Camera Blur");
					handle = cameraBlur.Filter(handle);
				}
				/*
				if(r_bloom)
					handle = GLBloomFilter(this).Filter(handle);*/
				if(r_lens){
					GLProfiler profiler(device, "Lens Filter");
					handle = GLLensFilter(this).Filter(handle);
				}
				
				if(r_lensFlare){
					GLProfiler profiler(device, "Lens Flare");
					device->BindFramebuffer(IGLDevice::Framebuffer, handle.GetFramebuffer());
					GLLensFlareFilter(this).Draw();
				}
				
				if(r_fxaa){
					GLProfiler profiler(device, "FXAA");
					handle = GLFXAAFilter(this).Filter(handle);
				}
			}
				
			if(r_srgb)
				device->Enable(IGLDevice::FramebufferSRGB, false);
			
			// copy buffer to WM given framebuffer
			{
				GLProfiler profiler(device, "Copying to WM-given Framebuffer");
				
				device->BindFramebuffer(IGLDevice::Framebuffer, 0);
				device->Enable(IGLDevice::Blend, false);
				device->Viewport(0, 0, handle.GetWidth(), handle.GetHeight());
				GLImage image(handle.GetTexture(),
							  device,
							  handle.GetWidth(),
							  handle.GetHeight(),
							  false);
				SetColor(MakeVector4(1, 1, 1, 1));
				DrawImage(&image, AABB2(0,handle.GetHeight(),handle.GetWidth(),-handle.GetHeight()));
				imageRenderer->Flush(); // must flush now because handle is released soon
			}
			
			handle.Release();
			fbManager->MakeSureAllBuffersReleased();
			
			// prepare for 2d drawing
			device->Enable(IGLDevice::Blend, true);
		}
		
		
//#pragma mark - 2D Drawings
		
		void GLRenderer::MultiplyScreenColor(spades::Vector3 color){
			SPADES_MARK_FUNCTION();
			imageRenderer->Flush();
			
			device->BlendFunc(IGLDevice::Zero,
							  IGLDevice::SrcColor);
			
			Vector4 col = {color.x, color.y, color.z, 1};
			
			// build vertices
			DebugLineVertex vertices[4];
			
			vertices[0] = DebugLineVertex::Create(MakeVector3(-1,-1,0),
													col);
			vertices[1] = DebugLineVertex::Create(MakeVector3(1,-1,0),
												  col);
			vertices[2] = DebugLineVertex::Create(MakeVector3(1,1,0),
												  col);
			vertices[3] = DebugLineVertex::Create(MakeVector3(-1,1,0),
												  col);
			
			
			GLProgram *program = RegisterProgram("Shaders/Basic.program");
			program->Use();
			
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			
			positionAttribute(program);
			colorAttribute(program);
			
			device->VertexAttribPointer(positionAttribute(),
										3, IGLDevice::FloatType,
										false, sizeof(DebugLineVertex),
										vertices);
			device->VertexAttribPointer(colorAttribute(),
										4, IGLDevice::FloatType,
										false, sizeof(DebugLineVertex),
										(const char*)vertices + sizeof(float) * 3);
			
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(colorAttribute(), true);
			
			static GLProgramUniform projectionViewMatrix("projectionViewMatrix");
			projectionViewMatrix(program);
			projectionViewMatrix.SetValue(Matrix4::Identity());
			
			device->DrawArrays(IGLDevice::TriangleFan,
							   0,
							   4);
			
			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(colorAttribute(), false);
			
			device->BlendFunc(IGLDevice::SrcAlpha,
							  IGLDevice::OneMinusSrcAlpha);
			
		}
		
		void GLRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  AABB2(outTopLeft.x, outTopLeft.y,
							image->GetWidth(),
							image->GetHeight()),
					  AABB2(0, 0,
							image->GetWidth(),
							image->GetHeight()));
		}
		
		void GLRenderer::DrawImage(client::IImage *image, const spades::AABB2 &outRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  outRect,
					  AABB2(0, 0,
							image->GetWidth(),
							image->GetHeight()));
		}
		
		void GLRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  AABB2(outTopLeft.x, outTopLeft.y,
							inRect.GetWidth(),
							inRect.GetHeight()),
					  inRect);
		}
		
		void GLRenderer::DrawImage(client::IImage *image,
								   const spades::AABB2 &outRect,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  Vector2::Make(outRect.GetMinX(), outRect.GetMinY()),
					  Vector2::Make(outRect.GetMaxX(), outRect.GetMinY()),
					  Vector2::Make(outRect.GetMinX(), outRect.GetMaxY()),
					  inRect);
		}
		
		void GLRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft,
								   const spades::Vector2 &outTopRight,
								   const spades::Vector2 &outBottomLeft,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			// d = a + (b - a) + (c - a)
			//   = b + c - a
			Vector2 outBottomRight = outTopRight + outBottomLeft - outTopLeft;
			GLImage *img = dynamic_cast<GLImage *>(image);
			if(!img){
				SPInvalidArgument("image");
			}
			
			imageRenderer->SetImage(img);
			
			imageRenderer->Add(outTopLeft.x, outTopLeft.y,
							   outTopRight.x, outTopRight.y,
							   outBottomRight.x, outBottomRight.y,
							   outBottomLeft.x, outBottomLeft.y,
							   inRect.GetMinX(), inRect.GetMinY(),
							   inRect.GetMaxX(), inRect.GetMinY(),
							   inRect.GetMaxX(), inRect.GetMaxY(),
							   inRect.GetMinX(), inRect.GetMaxY(),
							   drawColor.x, drawColor.y,
							   drawColor.z, drawColor.w);
			
		}
		
		void GLRenderer::DrawFlatGameMap(const spades::AABB2 &outRect,
										 const spades::AABB2 &inRect){
			if(flatMapRenderer)
				flatMapRenderer->Draw(outRect, inRect);
		}
		
		void GLRenderer::SetColor(spades::Vector4 col){
			drawColor = col;
		}
		
		void GLRenderer::FrameDone() {
			SPADES_MARK_FUNCTION();
			
			imageRenderer->Flush();
			lastTime = sceneDef.time;
			
		}
		
		void GLRenderer::Flip() {
			SPADES_MARK_FUNCTION();
			
			device->Swap();
		}
		
		Bitmap *GLRenderer::ReadBitmap() {
			SPADES_MARK_FUNCTION();
			Bitmap *bmp;
			bmp = new Bitmap(device->ScreenWidth(),
							 device->ScreenHeight());
			device->ReadPixels(0, 0, device->ScreenWidth(), device->ScreenHeight(), IGLDevice::RGBA, IGLDevice::UnsignedByte, bmp->GetPixels());
			return bmp;
		}
		
		void GLRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map){
			if(mapRenderer)
				mapRenderer->GameMapChanged(x, y, z, map);
			if(flatMapRenderer)
				flatMapRenderer->GameMapChanged(x, y, z, map);
			if(mapShadowRenderer)
				mapShadowRenderer->GameMapChanged(x, y, z, map);
			if(waterRenderer)
				waterRenderer->GameMapChanged(x, y, z, map);
			if(ambientShadowRenderer)
				ambientShadowRenderer->GameMapChanged(x, y, z, map);
		}
		
		bool GLRenderer::BoxFrustrumCull(const AABB3& box) {
			return PlaneCullTest(frustrum[0], box) &&
			PlaneCullTest(frustrum[1], box) &&
			PlaneCullTest(frustrum[2], box) &&
			PlaneCullTest(frustrum[3], box) &&
			PlaneCullTest(frustrum[4], box) &&
			PlaneCullTest(frustrum[5], box);
		}
		bool GLRenderer::SphereFrustrumCull(const Vector3& center,
												float radius) {
			for(int i = 0; i < 6; i++){
				if(frustrum[i].GetDistanceTo(center) < -radius)
					return false;
			}
			return true;
		}
		
	}
}