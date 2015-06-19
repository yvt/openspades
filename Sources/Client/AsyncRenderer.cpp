/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include "AsyncRenderer.h"
#include <new>
#include "../Core/Exception.h"
#include "../Core/Debug.h"
#include "../Core/Mutex.h"
#include "../Core/AutoLocker.h"
#include "IImage.h"
#include "IModel.h"
#include <stdint.h>

namespace spades {
	namespace client {
		
		
		
#pragma mark - Commands
		
		class Command {
		public:
			uint16_t cmdSize;
			
			virtual void Execute(IRenderer *) = 0;
		};
		
		namespace rcmds {
			class Init: public Command {
			public:
				virtual void Execute(IRenderer *r) {
					SPADES_MARK_FUNCTION();
					r->Init();
				}
			};
			class Shutdown: public Command {
			public:
				virtual void Execute(IRenderer *r) {
					SPADES_MARK_FUNCTION();
					r->Shutdown();
				}
			};
			class SetGameMap: public Command {
			public:
				GameMap *map;
				virtual void Execute(IRenderer *r) {
					SPADES_MARK_FUNCTION();
					r->SetGameMap(map);
				}
			};
			class SetFogDistance: public Command {
			public:
				float v;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->SetFogDistance(v);
				}
			};
			class SetFogColor: public Command {
			public:
				Vector3 v;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->SetFogColor(v);
				}
			};
			class StartScene: public Command {
			public:
				SceneDefinition def;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->StartScene(def);
				}
			};
			class AddLight: public Command {
			public:
				IImage *img;
				DynamicLightParam def;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					if(img){
						def.image = img; img = NULL;
					}
					try{
						r->AddLight(def);
					}catch(...){
						if(img) img->Release();
						throw;
					}
					if(img){
						img->Release();
					}
				}
			};
			class RenderModel: public Command {
			public:
				IModel *model;
				ModelRenderParam param;
				virtual void Execute(IRenderer *r){
					try {
						r->RenderModel(model, param);
					}catch(...){
						model->Release(); model = NULL;
						throw;
					}
					model->Release(); model = NULL;
				}
			};
			class AddDebugLine: public Command {
			public:
				Vector3 a, b;
				Vector4 color;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->AddDebugLine(a, b, color);
				}
			};
			class AddSprite: public Command {
			public:
				IImage *img;
				Vector3 center;
				float radius, rotation;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					try {
						r->AddSprite(img, center, radius, rotation);
					}catch(...){
						img->Release(); img = NULL;
						throw;
					}
					img->Release(); img = NULL;
				}
			};
			class AddLongSprite: public Command {
			public:
				IImage *img;
				Vector3 p1, p2;
				float radius;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					try {
						r->AddLongSprite(img, p1, p2, radius);
					}catch(...){
						img->Release(); img = NULL;
						throw;
					}
					img->Release(); img = NULL;
				}
			};
			class EndScene: public Command {
			public:
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->EndScene();
				}
			};
			class MultiplyScreenColor: public Command {
			public:
				Vector3 v;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->MultiplyScreenColor(v);
				}
			};
			class SetColor: public Command {
			public:
				Vector4 v;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->SetColor(v);
				}
			};
			class SetColorAlphaPremultiplied: public Command {
			public:
				Vector4 v;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->SetColorAlphaPremultiplied(v);
				}
			};
			class DrawImage: public Command {
			public:
				IImage *img;
				Vector2 outTopLeft;
				Vector2 outTopRight;
				Vector2 outBottomLeft;
				AABB2 inRect;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					try{
						r->DrawImage(img, outTopLeft, outTopRight, outBottomLeft,
									 inRect);
					}catch(...){
						if(img) img->Release(); img = NULL;
						throw;
					}
					if(img) img->Release(); img = NULL;
				}
			};
			class DrawFlatGameMap: public Command {
			public:
				AABB2 outRect, inRect;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->DrawFlatGameMap(outRect, inRect);
				}
			};
			class FrameDone: public Command {
			public:
				AsyncRenderer *arenderer;
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->FrameDone();
				}
			};
			class Flip: public Command {
			public:
				virtual void Execute(IRenderer *r){
					SPADES_MARK_FUNCTION();
					r->Flip();
				}
			};
		};
		
#pragma mark - Command Buffer
		
		class AsyncRenderer::CmdBufferGenerator {
		public:
			std::vector<char> buffer;
			
			void Clear(){
				buffer.clear();
			}
			
			template<typename T>
			T *AllocCommand() {
				SPADES_MARK_FUNCTION();
				
				size_t size = sizeof(T);
				size_t pos = buffer.size();
				buffer.resize(buffer.size() + size);
				
				void *dat = buffer.data() + pos;
				T *cmd = new(dat) T;
				cmd->cmdSize = sizeof(T);
				return cmd;
			}
		};
		
		class AsyncRenderer::CmdBufferReader {
			std::vector<char> buffer;
			size_t pos;
		public:
			CmdBufferReader(const std::vector<char>& buf):
			buffer(buf), pos(0){
			}
			Command *NextCommand() {
				SPADES_MARK_FUNCTION();
				
				if(pos >= buffer.size()){
					return NULL;
				}
				Command *cmd = reinterpret_cast<Command *>(buffer.data() + pos);
				pos += cmd->cmdSize;
				if(pos > buffer.size()) {
					SPRaise("Truncated render command buffer");
				}
				return cmd;
			}
		};
		
		class AsyncRenderer::RenderDispatch:
		public ConcurrentDispatch{
			AsyncRenderer *renderer;
		public:
			std::vector<char> buffer;
			
			RenderDispatch(AsyncRenderer *renderer):
			renderer(renderer){
				
			}
			virtual void Run() {
				SPADES_MARK_FUNCTION();
				
				CmdBufferReader reader(buffer);
				Command *cmd;
				while((cmd = reader.NextCommand()) != NULL){
					cmd->Execute(renderer->base);
				}
			}
		};
		
		AsyncRenderer::AsyncRenderer(IRenderer *base,
									 DispatchQueue *queue):
		base(base), queue(queue){
			generator = new CmdBufferGenerator();
			dispatch = new RenderDispatch(this);
		}
		
		AsyncRenderer::~AsyncRenderer(){
			Sync();
			
			delete dispatch;
			delete generator;
		}
		
		void AsyncRenderer::FlushCommands(){
			if(generator->buffer.empty())
				return;
			
			dispatch->Join();
			dispatch->buffer = std::move(generator->buffer);
			generator->Clear();
			dispatch->StartOn(queue);
		}
		
		void AsyncRenderer::Sync(){
			FlushCommands();
			dispatch->Join();
		}
		
#pragma mark - General COmmands
		
		IImage *AsyncRenderer::RegisterImage(const char *filename) {
			SPADES_MARK_FUNCTION();
			
			class RegisterImageDispatch: public ConcurrentDispatch {
				IRenderer *base;
				const char *fn;
				IImage *result;
				std::string error;
			public:
				RegisterImageDispatch(IRenderer *base, const char *fn):
				base(base), fn(fn), result(NULL){}
				virtual void Run(){
					try{
						result = base->RegisterImage(fn);
					}catch(const std::exception& ex){
						error = ex.what();
					}
				}
				IImage *GetResult() {
					if(!error.empty()){
						SPRaise("Error while RegisterImageDispatch:\n%s", error.c_str());
					}else{
						return result;
					}
				}
			};
			
			std::map<std::string, IImage *>::iterator it = images.find(filename);
			if(it == images.end()) {
				FlushCommands();
				RegisterImageDispatch dispatch(base, filename);
				dispatch.StartOn(queue);
				dispatch.Join();
				
				IImage *img = dispatch.GetResult();
				images[filename] = img;
				img->AddRef();
				return img;
			}
			it->second->AddRef();
			return it->second;
		}
		
		void AsyncRenderer::Init() {
			SPADES_MARK_FUNCTION();
			generator->AllocCommand<rcmds::Init>();
		}
		
		void AsyncRenderer::Shutdown() {
			SPADES_MARK_FUNCTION();
			generator->AllocCommand<rcmds::Shutdown>();
		}
		
		IImage *AsyncRenderer::CreateImage(spades::Bitmap *bmp) {
			SPADES_MARK_FUNCTION();
			
			class CreateImageDispatch: public ConcurrentDispatch {
				IRenderer *base;
				Bitmap *bmp;
				IImage *result;
				std::string error;
			public:
				CreateImageDispatch(IRenderer *base, Bitmap *bmp):
				base(base), bmp(bmp), result(NULL){}
				virtual void Run(){
					try{
						result = base->CreateImage(bmp);
					}catch(const std::exception& ex){
						error = ex.what();
					}
				}
				IImage *GetResult() {
					if(!error.empty()){
						SPRaise("Error while CreateImageDispatch:\n%s", error.c_str());
					}else{
						return result;
					}
				}
			};
			
			FlushCommands();
			CreateImageDispatch dispatch(base, bmp);
			dispatch.StartOn(queue);
			dispatch.Join();
			IImage *img = dispatch.GetResult();
			return img;
		}
		
		IModel *AsyncRenderer::RegisterModel(const char *filename) {
			SPADES_MARK_FUNCTION();
			
			class RegisterModelDispatch: public ConcurrentDispatch {
				IRenderer *base;
				const char *fn;
				IModel *result;
				std::string error;
			public:
				RegisterModelDispatch(IRenderer *base, const char *fn):
				base(base), fn(fn), result(NULL){}
				virtual void Run(){
					try{
						result = base->RegisterModel(fn);
					}catch(const std::exception& ex){
						error = ex.what();
					}
				}
				IModel *GetResult() {
					if(!error.empty()){
						SPRaise("Error while RegisterImageDispatch:\n%s", error.c_str());
					}else{
						return result;
					}
				}
			};
			
			std::map<std::string, IModel *>::iterator it = models.find(filename);
			if(it == models.end()) {
				FlushCommands();
				RegisterModelDispatch dispatch(base, filename);
				dispatch.StartOn(queue);
				dispatch.Join();
				IModel *img = dispatch.GetResult();
				models[filename] = img;
				img->AddRef();
				return img;
			}
			it->second->AddRef();
			return it->second;
		}
		
		IModel *AsyncRenderer::CreateModel(spades::VoxelModel *bmp) {
			SPADES_MARK_FUNCTION();
			
			class CreateModelDispatch: public ConcurrentDispatch {
				IRenderer *base;
				VoxelModel *bmp;
				IModel *result;
				std::string error;
			public:
				CreateModelDispatch(IRenderer *base, VoxelModel *bmp):
				base(base), bmp(bmp), result(NULL){}
				virtual void Run(){
					try{
						result = base->CreateModel(bmp);
					}catch(const std::exception& ex){
						error = ex.what();
					}
				}
				IModel *GetResult() {
					if(!error.empty()){
						SPRaise("Error while CreateImageDispatch:\n%s", error.c_str());
					}else{
						return result;
					}
				}
			};
			
			FlushCommands();
			CreateModelDispatch dispatch(base, bmp);
			dispatch.StartOn(queue);
			dispatch.Join();
			IModel *img = dispatch.GetResult();
			return img;
		}
		
		void AsyncRenderer::SetGameMap(GameMap *gm) {
			SPADES_MARK_FUNCTION();
			rcmds::SetGameMap *cmd = generator->AllocCommand<rcmds::SetGameMap>();
			cmd->map = gm;
			Sync();
		}
		
		void AsyncRenderer::SetFogDistance(float distance) {
			SPADES_MARK_FUNCTION();
			rcmds::SetFogDistance *cmd = generator->AllocCommand<rcmds::SetFogDistance>();
			cmd->v = distance;
		}
		void AsyncRenderer::SetFogColor(Vector3 v) {
			SPADES_MARK_FUNCTION();
			rcmds::SetFogColor *cmd = generator->AllocCommand<rcmds::SetFogColor>();
			cmd->v = v;
		}
		
		void AsyncRenderer::StartScene(const SceneDefinition& def) {
			SPADES_MARK_FUNCTION();
			rcmds::StartScene *cmd = generator->AllocCommand<rcmds::StartScene>();
			cmd->def = def;
		}
		
		void AsyncRenderer::AddLight(const client::DynamicLightParam& light) {
			SPADES_MARK_FUNCTION();
			rcmds::AddLight *cmd = generator->AllocCommand<rcmds::AddLight>();
			if(light.image){
				cmd->img = light.image;
				cmd->img->AddRef();
			}else{
				cmd->img = NULL;
			}
			cmd->def = light;
		}
		
		void AsyncRenderer::RenderModel(IModel *m, const ModelRenderParam&p) {
			SPADES_MARK_FUNCTION();
			rcmds::RenderModel *cmd = generator->AllocCommand<rcmds::RenderModel>();
			cmd->model = m;
			m->AddRef();
			cmd->param = p;
		}
		
		void AsyncRenderer::AddDebugLine(Vector3 a, Vector3 b, Vector4 color){
			SPADES_MARK_FUNCTION();
			rcmds::AddDebugLine *cmd = generator->AllocCommand<rcmds::AddDebugLine>();
			cmd->a = a; cmd->b = b;
			cmd->color = color;
		}
		
		void AsyncRenderer::AddSprite(IImage *img, Vector3 center,
									  float radius, float rotation){
			SPADES_MARK_FUNCTION();
			rcmds::AddSprite *cmd = generator->AllocCommand<rcmds::AddSprite>();
			cmd->img = img;
			img->AddRef();
			cmd->center = center;
			cmd->radius = radius;
			cmd->rotation = rotation;
		}
		
		void AsyncRenderer::AddLongSprite(IImage *img, Vector3 p1, Vector3 p2,
									  float radius){
			SPADES_MARK_FUNCTION();
			rcmds::AddLongSprite *cmd = generator->AllocCommand<rcmds::AddLongSprite>();
			cmd->img = img;
			img->AddRef();
			cmd->p1 = p1;
			cmd->p2 = p2;
			cmd->radius = radius;
		}
		
		void AsyncRenderer::EndScene() {
			SPADES_MARK_FUNCTION();
			generator->AllocCommand<rcmds::EndScene>();
		}

		void AsyncRenderer::MultiplyScreenColor(Vector3 v) {
			SPADES_MARK_FUNCTION();
			rcmds::MultiplyScreenColor *cmd = generator->AllocCommand<rcmds::MultiplyScreenColor>();
			cmd->v = v;
		}
		
		void AsyncRenderer::SetColor(Vector4 c) {
			SPADES_MARK_FUNCTION();
			rcmds::SetColor *cmd = generator->AllocCommand<rcmds::SetColor>();
			cmd->v = c;
		}
		
		void AsyncRenderer::SetColorAlphaPremultiplied(Vector4 c) {
			SPADES_MARK_FUNCTION();
			rcmds::SetColorAlphaPremultiplied *cmd = generator->AllocCommand<rcmds::SetColorAlphaPremultiplied>();
			cmd->v = c;
		}
		
		void AsyncRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  AABB2(outTopLeft.x, outTopLeft.y,
							image?image->GetWidth():0,
							image?image->GetHeight():0),
					  AABB2(0, 0,
							image?image->GetWidth():0,
							image?image->GetHeight():0));
		}
		
		void AsyncRenderer::DrawImage(client::IImage *image, const spades::AABB2 &outRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  outRect,
					  AABB2(0, 0,
							image ? image->GetWidth() : 0,
							image ? image->GetHeight(): 0));
		}
		
		void AsyncRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  AABB2(outTopLeft.x, outTopLeft.y,
							inRect.GetWidth(),
							inRect.GetHeight()),
					  inRect);
		}
		
		void AsyncRenderer::DrawImage(client::IImage *image,
								   const spades::AABB2 &outRect,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  Vector2::Make(outRect.GetMinX(), outRect.GetMinY()),
					  Vector2::Make(outRect.GetMaxX(), outRect.GetMinY()),
					  Vector2::Make(outRect.GetMinX(), outRect.GetMaxY()),
					  inRect);
		}
		
		void AsyncRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft,
								   const spades::Vector2 &outTopRight,
								   const spades::Vector2 &outBottomLeft,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			rcmds::DrawImage *cmd = generator->AllocCommand<rcmds::DrawImage>();
			cmd->img = image;
			if(image) image->AddRef();
			cmd->outTopLeft = outTopLeft;
			cmd->outTopRight = outTopRight;
			cmd->outBottomLeft = outBottomLeft;
			cmd->inRect = inRect;
		}
		
		void AsyncRenderer::DrawFlatGameMap(const spades::AABB2 &outRect,
											const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			rcmds::DrawFlatGameMap *cmd = generator->AllocCommand<rcmds::DrawFlatGameMap>();
			cmd->outRect = outRect;
			cmd->inRect = inRect;
		}
		
		void AsyncRenderer::FrameDone() {
			generator->AllocCommand<rcmds::FrameDone>()->arenderer = this;
		}
		
		void AsyncRenderer::Flip() {
			generator->AllocCommand<rcmds::Flip>();
			FlushCommands();
		}
		
		Bitmap *AsyncRenderer::ReadBitmap() {
			FlushCommands();
			
			class ReadBitmapDispatch: public ConcurrentDispatch {
				IRenderer *base;
				Bitmap *result;
				std::string error;
			public:
				ReadBitmapDispatch(IRenderer *base):
				base(base), result(NULL){}
				virtual void Run(){
					try{
						result = base->ReadBitmap();
					}catch(const std::exception& ex){
						error = ex.what();
					}
				}
				Bitmap *GetResult() {
					if(!error.empty()){
						SPRaise("Error while CreateImageDispatch:\n%s", error.c_str());
					}else{
						return result;
					}
				}
			};
			
			ReadBitmapDispatch disp(base);
			disp.StartOn(queue);
			disp.Join();
			return disp.GetResult();
		}
		
		float AsyncRenderer::ScreenWidth() {
			return base->ScreenWidth();
		}
		
		float AsyncRenderer::ScreenHeight(){
			return base->ScreenHeight();
		}
		
		
	}
}

