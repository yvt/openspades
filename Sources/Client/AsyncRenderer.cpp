//
//  AsyncRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 7/29/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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
		
#pragma mark - Resources
		
		class AsyncImage: public IImage {
		public:
			virtual ~AsyncImage(){}
			virtual IImage *GetImage() = 0;
			virtual float GetWidth() { return GetImage()->GetWidth(); }
			virtual float GetHeight() { return GetImage()->GetHeight(); }
			virtual AsyncImage *Copy() = 0;
		};
		
		class SharedAsyncImage: public AsyncImage {
			IImage *img;
		public:
			SharedAsyncImage(IImage *img):img(img){}
			virtual ~SharedAsyncImage() {}
			virtual IImage *GetImage() { return img; }
			virtual AsyncImage *Copy() {
				return new SharedAsyncImage(img);
			}
		};
		
		class TemporaryAsyncImage: public AsyncImage {
			struct Data {
				int refCount;
				AsyncRenderer *arenderer;
				Mutex mutex;
				IImage *img;
				Data(IImage *img):
				refCount(1), img(img){}
				Data *Retain() {
					AutoLocker locker(&mutex);
					refCount++;
					return this;
				}
				Data *Release() {
					AutoLocker locker(&mutex);
					refCount--;
					if(refCount > 0){
						return this;
					}else{
						arenderer->deletedImages.push_back(img);
						delete this;
						return NULL;
					}
				}
			};
			Data *data;
		public:
			TemporaryAsyncImage(IImage *img, AsyncRenderer *r):
			data(new Data(img)){
				data->arenderer = r;
			}
			TemporaryAsyncImage(Data *d):
			data(d){}
			virtual ~TemporaryAsyncImage() { data->Release(); }
			virtual IImage *GetImage() { return data->img; }
			virtual AsyncImage *Copy() {
				return new TemporaryAsyncImage(data->Retain());
			}
		};
		
		class AsyncModel: public IModel {
		public:
			virtual ~AsyncModel(){}
			virtual IModel *GetModel() = 0;
			virtual AsyncModel *Copy() = 0;
		};
		
		class SharedAsyncModel: public AsyncModel {
			IModel *model;
		public:
			SharedAsyncModel(IModel *model):model(model){}
			virtual ~SharedAsyncModel() {}
			virtual IModel *GetModel() { return model; }
			virtual AsyncModel *Copy() {
				return new SharedAsyncModel(model);
			}
		};
		
		class TemporaryAsyncModel: public AsyncModel {
			struct Data {
				int refCount;
				AsyncRenderer *arenderer;
				Mutex mutex;
				IModel *model;
				Data(IModel *model):
				refCount(1), model(model){}
				Data *Retain() {
					AutoLocker locker(&mutex);
					refCount++;
					return this;
				}
				Data *Release() {
					AutoLocker locker(&mutex);
					refCount--;
					if(refCount > 0){
						return this;
					}else{
						arenderer->deletedModels.push_back(model);
						delete this;
						return NULL;
					}
				}
			};
			Data *data;
		public:
			TemporaryAsyncModel(IModel *model, AsyncRenderer *r):
			data(new Data(model)){
				data->arenderer = r;
			}
			TemporaryAsyncModel(Data *d):
			data(d){}
			virtual ~TemporaryAsyncModel() { data->Release(); }
			virtual IModel *GetModel() { return data->model; }
			virtual AsyncModel *Copy() {
				return new TemporaryAsyncModel(data->Retain());
			}
		};
		
		
		
#pragma mark - Commands
		
		class Command {
		public:
			uint16_t cmdSize;
			
			virtual void Execute(IRenderer *) = 0;
		};
		
		namespace rcmds {
			class SetGameMap: public Command {
			public:
				GameMap *map;
				virtual void Execute(IRenderer *r) {
					r->SetGameMap(map);
				}
			};
			class SetFogDistance: public Command {
			public:
				float v;
				virtual void Execute(IRenderer *r){
					r->SetFogDistance(v);
				}
			};
			class SetFogColor: public Command {
			public:
				Vector3 v;
				virtual void Execute(IRenderer *r){
					r->SetFogColor(v);
				}
			};
			class StartScene: public Command {
			public:
				SceneDefinition def;
				virtual void Execute(IRenderer *r){
					r->StartScene(def);
				}
			};
			class AddLight: public Command {
			public:
				AsyncImage *img;
				DynamicLightParam def;
				virtual void Execute(IRenderer *r){
					if(img){
						def.image = img->GetImage();
						delete img;
					}
					r->AddLight(def);
				}
			};
			class RenderModel: public Command {
			public:
				AsyncModel *model;
				ModelRenderParam param;
				virtual void Execute(IRenderer *r){
					r->RenderModel(model->GetModel(), param);
					delete model;
				}
			};
			class AddDebugLine: public Command {
			public:
				Vector3 a, b;
				Vector4 color;
				virtual void Execute(IRenderer *r){
					r->AddDebugLine(a, b, color);
				}
			};
			class AddSprite: public Command {
			public:
				AsyncImage *img;
				Vector3 center;
				float radius, rotation;
				virtual void Execute(IRenderer *r){
					r->AddSprite(img->GetImage(), center, radius, rotation);
					delete img;
				}
			};
			class EndScene: public Command {
			public:
				virtual void Execute(IRenderer *r){
					r->EndScene();
				}
			};
			class MultiplyScreenColor: public Command {
			public:
				Vector3 v;
				virtual void Execute(IRenderer *r){
					r->MultiplyScreenColor(v);
				}
			};
			class SetColor: public Command {
			public:
				Vector4 v;
				virtual void Execute(IRenderer *r){
					r->SetColor(v);
				}
			};
			class DrawImage: public Command {
			public:
				AsyncImage *img;
				Vector2 outTopLeft;
				Vector2 outTopRight;
				Vector2 outBottomLeft;
				AABB2 inRect;
				virtual void Execute(IRenderer *r){
					r->DrawImage(img->GetImage(), outTopLeft, outTopRight, outBottomLeft,
								 inRect);
					delete img;
				}
			};
			class DrawFlatGameMap: public Command {
			public:
				AABB2 outRect, inRect;
				virtual void Execute(IRenderer *r){
					r->DrawFlatGameMap(outRect, inRect);
				}
			};
			class FrameDone: public Command {
			public:
				AsyncRenderer *arenderer;
				virtual void Execute(IRenderer *r){
					r->FrameDone();
					
					arenderer->DeleteDeferredResources();
				}
			};
			class Flip: public Command {
			public:
				virtual void Execute(IRenderer *r){
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
			DeleteDeferredResources();
			
			for(std::map<std::string, IImage *>::iterator it =
				images.begin(); it != images.end(); it++)
				delete it->second;
			
			for(std::map<std::string, IModel *>::iterator it =
				models.begin(); it != models.end(); it++)
				delete it->second;
			
			delete dispatch;
			delete generator;
		}
		
		void AsyncRenderer::FlushCommands(){
			if(generator->buffer.empty())
				return;
			
			dispatch->Join();
			dispatch->buffer = generator->buffer;
			generator->Clear();
			dispatch->StartOn(queue);
		}
		
		void AsyncRenderer::Sync(){
			FlushCommands();
			dispatch->Join();
		}
		
		void AsyncRenderer::DeleteDeferredResources(){
			
			
			for(std::vector<IModel *>::iterator it = deletedModels.begin(); it != deletedModels.end(); it++)
				delete *it;
			deletedModels.clear();
			
			for(std::vector<IImage *>::iterator it = deletedImages.begin(); it != deletedImages.end(); it++)
				delete *it;
			deletedImages.clear();
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
				RegisterImageDispatch dispatch(base, filename);
				dispatch.StartOn(queue);
				dispatch.Join();
				AsyncImage *img = new SharedAsyncImage(dispatch.GetResult());
				images[filename] = img;
				return img;
			}
			return it->second;
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
			
			CreateImageDispatch dispatch(base, bmp);
			dispatch.StartOn(queue);
			dispatch.Join();
			IImage *img = dispatch.GetResult();
			AsyncImage *i = new TemporaryAsyncImage(img, this);
			return i;
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
				RegisterModelDispatch dispatch(base, filename);
				dispatch.StartOn(queue);
				dispatch.Join();
				AsyncModel *img = new SharedAsyncModel(dispatch.GetResult());
				models[filename] = img;
				return img;
			}
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
			
			CreateModelDispatch dispatch(base, bmp);
			dispatch.StartOn(queue);
			dispatch.Join();
			IModel *img = dispatch.GetResult();
			AsyncModel *i = new TemporaryAsyncModel(img, this);
			return i;
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
				cmd->img = static_cast<AsyncImage *>(light.image)->Copy();
			}else{
				cmd->img = NULL;
			}
			cmd->def = light;
		}
		
		void AsyncRenderer::RenderModel(IModel *m, const ModelRenderParam&p) {
			SPADES_MARK_FUNCTION();
			rcmds::RenderModel *cmd = generator->AllocCommand<rcmds::RenderModel>();
			cmd->model = static_cast<AsyncModel *>(m)->Copy();
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
			cmd->img = static_cast<AsyncImage *>(img)->Copy();
			cmd->center = center;
			cmd->radius = radius;
			cmd->rotation = rotation;
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
		
		void AsyncRenderer::DrawImage(client::IImage *image,
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
		
		void AsyncRenderer::DrawImage(client::IImage *image, const spades::AABB2 &outRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  outRect,
					  AABB2(0, 0,
							image->GetWidth(),
							image->GetHeight()));
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
			
			AsyncImage *img = dynamic_cast<AsyncImage *>(image);
			if(!img){
				SPInvalidArgument("image");
			}
			
			rcmds::DrawImage *cmd = generator->AllocCommand<rcmds::DrawImage>();
			cmd->img = img->Copy();
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

