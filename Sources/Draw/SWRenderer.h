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

#include <array>
#include <map>
#include <memory>
#include <vector>

#include "SWFeatureLevel.h"
#include <Client/IGameMapListener.h>
#include <Client/IRenderer.h>
#include <Client/SceneDefinition.h>
#include <Core/Math.h>
#include <Core/Stopwatch.h>

namespace spades {
	namespace draw {

		class SWPort;
		class SWImageManager;
		class SWModelManager;
		class SWImageRenderer;
		class SWModelRenderer;
		class SWFlatMapRenderer;
		class SWMapRenderer;
		class SWImage;
		class SWModel;

		class SWRenderer : public client::IRenderer, public client::IGameMapListener {
			friend class SWFlatMapRenderer;
			friend class SWModelRenderer;
			friend class SWMapRenderer;

			SWFeatureLevel featureLevel;

			Handle<SWPort> port;
			Handle<client::GameMap> map;

			Handle<Bitmap> fb;
			std::vector<float> depthBuffer;

			std::shared_ptr<SWImageManager> imageManager;
			std::shared_ptr<SWModelManager> modelManager;

			std::shared_ptr<SWImageRenderer> imageRenderer;
			std::shared_ptr<SWModelRenderer> modelRenderer;

			std::shared_ptr<SWFlatMapRenderer> flatMapRenderer;
			std::shared_ptr<SWMapRenderer> mapRenderer;

			struct Sprite {
				Handle<SWImage> img;
				Vector3 center;
				float radius;
				float rotation;
				Vector4 color;
			};
			std::vector<Sprite> sprites;

			struct LongSprite {
				Handle<SWImage> img;
				Vector3 start;
				Vector3 end;
				float radius;
				Vector4 color;
			};
			std::vector<LongSprite> longSprites;

			struct Model {
				Handle<SWModel> model;
				client::ModelRenderParam param;
			};
			std::vector<Model> models;

			struct DynamicLight {
				client::DynamicLightParam param;
				int minX, maxX, minY, maxY;
			};
			std::vector<DynamicLight> lights;

			bool inited;
			bool sceneUsedInThisFrame;

			client::SceneDefinition sceneDef;
			std::array<Plane3, 6> frustrum;

			struct DebugLine {
				Vector3 v1, v2;
				Vector4 color;
			};

			std::vector<DebugLine> debugLines;

			float fogDistance;
			Vector3 fogColor;

			Matrix4 projectionMatrix;
			Matrix4 viewMatrix;
			Matrix4 projectionViewMatrix;

			Vector4 drawColorAlphaPremultiplied;
			bool legacyColorPremultiply;

			unsigned int lastTime;

			Stopwatch renderStopwatch;

			bool duringSceneRendering;

			void BuildProjectionMatrix();
			void BuildView();
			void BuildFrustrum();

			void RenderDebugLines();

			void RenderObjects();

			void EnsureInitialized();
			void EnsureSceneStarted();
			void EnsureSceneNotStarted();
			void EnsureValid();

			void SetFramebuffer(Bitmap *);

			template <SWFeatureLevel> void ApplyFog();

			template <SWFeatureLevel> void ApplyDynamicLight(const DynamicLight &);

		protected:
			~SWRenderer();

		public:
			SWRenderer(Handle<SWPort> port, SWFeatureLevel featureLevel = DetectFeatureLevel());

			void Init() override;
			void Shutdown() override;

			Handle<client::IImage> RegisterImage(const char *filename) override;
			Handle<client::IModel> RegisterModel(const char *filename) override;

			Handle<client::IImage> CreateImage(Bitmap &) override;
			Handle<client::IModel> CreateModel(VoxelModel &) override;

			void ClearCache() override;

			void SetGameMap(stmp::optional<client::GameMap &>) override;
			void SetFogColor(Vector3 v) override;
			void SetFogDistance(float f) override { fogDistance = f; }

			Vector3 GetFogColor() { return fogColor; }
			float GetFogDistance() { return fogDistance; }

			void StartScene(const client::SceneDefinition &) override;

			void RenderModel(client::IModel &, const client::ModelRenderParam &) override;

			void AddLight(const client::DynamicLightParam &light) override;

			void AddDebugLine(Vector3 a, Vector3 b, Vector4 color) override;

			void AddSprite(client::IImage &, Vector3 center, float radius, float rotation) override;
			void AddLongSprite(client::IImage &, Vector3 p1, Vector3 p2, float radius) override;

			void EndScene() override;

			void MultiplyScreenColor(Vector3) override;

			void SetColor(Vector4) override;
			void SetColorAlphaPremultiplied(Vector4) override;

			void DrawImage(stmp::optional<client::IImage &>, const Vector2 &outTopLeft) override;
			void DrawImage(stmp::optional<client::IImage &>, const AABB2 &outRect) override;
			void DrawImage(stmp::optional<client::IImage &>, const Vector2 &outTopLeft,
			               const AABB2 &inRect) override;
			void DrawImage(stmp::optional<client::IImage &>, const AABB2 &outRect,
			               const AABB2 &inRect) override;
			void DrawImage(stmp::optional<client::IImage &>, const Vector2 &outTopLeft,
			               const Vector2 &outTopRight, const Vector2 &outBottomLeft,
			               const AABB2 &inRect) override;

			void DrawFlatGameMap(const AABB2 &outRect, const AABB2 &inRect) override;

			void FrameDone() override;
			void Flip() override;
			Handle<Bitmap> ReadBitmap() override;

			float ScreenWidth() override;
			float ScreenHeight() override;

			const Matrix4 &GetProjectionMatrix() const { return projectionMatrix; }
			const Matrix4 &GetProjectionViewMatrix() const { return projectionViewMatrix; }
			const Matrix4 &GetViewMatrix() const { return viewMatrix; }

			void GameMapChanged(int x, int y, int z, client::GameMap *) override;

			const client::SceneDefinition &GetSceneDef() const { return sceneDef; }

			bool BoxFrustrumCull(const AABB3 &);
			bool SphereFrustrumCull(const Vector3 &center, float radius);
		};
	} // namespace draw
} // namespace spades
