#include "ModelRenderParam.as"
#include "DynamicLightParam.as"
#include "SceneDefinition.as"
#include "IModel.as"
#include "IImage.as"

namespace spades {
	interface IRenderer {
	
		void Init();
	
		IImage @RegisterImage(string);
		IModel @RegisterModel(string);
	
		IImage @CreateImage(Bitmap@);
		IModel @CreateModel(VoxelModel@);
		
		void SetGameMap(GameMap@);
		
		float FogDistance { set; }
		Vector3 FogColor { set; }
		
		void StartScene(const SceneDefinition@);
		
		void AddLight(const DynamicLightParam@);
		
		void RenderModel(IModel@, const ModelRenderParam@);
		void AddDebugLine(Vector3 a, Vector3 b, Vector4 color);
		
		void AddSprite(IImage@, Vector3 center, float radius, float rotation);
		void AddLongSprite(IImage@, Vector3 p1, Vector3 p2, float radius);
		
		void EndScene();
		
		void MultiplyScreenColor(Vector3);
		void SetColor(Vector4);
		void DrawImage(IImage@, Vector2 outTopLeft);
		void DrawImage(IImage@, AABB2 outRect);
		void DrawImage(IImage@, Vector2 outTopLeft, AABB2 inRect);
		void DrawImage(IImage@, AABB2 outRect, AABB2 inRect);
		void DrawImage(IImage@, Vector2 outTopLeft, Vector2 outTopRight,
					Vector2 outBottomLeft, AABB2 inRect);
		
		void DrawFlatGameMap(AABB2 outRect, AABB2 inRect);
		
		void FrameDone();
		void Flip();
		
		Bitmap@ ReadBitmap();
		
		float ScreenWidth { get; }
		float ScreenHeight { get; }
		
		
	}
}
