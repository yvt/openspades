#include "/Base/IRenderer.as"

namespace spades {
	class NativeRenderer: IRenderer {
		private LowLevelNativeRenderer@ inner;
	
		NativeRenderer(LowLevelNativeRenderer@ ll) {
			@this.inner = ll;
		}
		
		void Init() {
		}
		
		IImage @RegisterImage(string filename){
			return NativeImage(inner.RegisterImage(filename));
		}
		IModel @RegisterModel(string filename){
			return NativeModel(inner.RegisterModel(filename));
		}
	
		IImage @CreateImage(Bitmap@ bmp){
			return NativeImage(inner.CreateImage(bmp));
		}
		IModel @CreateModel(VoxelModel@ model){
			return NativeModel(inner.CreateModel(model));
		}
		
		void SetGameMap(GameMap@) {
			NotImplemented();
		}
		
		float FogDistance { 
			set {
				NotImplemented();
			}
		}
		Vector3 FogColor { 
			set {
				NotImplemented();
			} 
		}
		
		void StartScene(const SceneDefinition@) {
			NotImplemented();
		}
		
		void AddLight(const DynamicLightParam@){
			NotImplemented();
		}
		
		void RenderModel(IModel@, const ModelRenderParam@) {
			NotImplemented();
		}
		void AddDebugLine(Vector3 a, Vector3 b, Vector4 color){
			NotImplemented();
		}
		
		void AddSprite(IImage@, Vector3 center, float radius, float rotation) {
			NotImplemented();
		}
		void AddLongSprite(IImage@, Vector3 p1, Vector3 p2, float radius) {
			NotImplemented();
		}
		
		void EndScene(){
			NotImplemented();
		}
		
		void MultiplyScreenColor(Vector3) {
			NotImplemented();
		}
		void SetColor(Vector4){
			NotImplemented();
		}
		void DrawImage(IImage@, Vector2 outTopLeft) {
			NotImplemented();
		}
		void DrawImage(IImage@, AABB2 outRect) {
			NotImplemented();
		}
		void DrawImage(IImage@, Vector2 outTopLeft, AABB2 inRect) {
			NotImplemented();
		}
		void DrawImage(IImage@, AABB2 outRect, AABB2 inRect) {
			NotImplemented();
		}
		void DrawImage(IImage@, Vector2 outTopLeft, Vector2 outTopRight,
					Vector2 outBottomLeft, AABB2 inRect) {
			NotImplemented();	
		}
		
		void DrawFlatGameMap(AABB2 outRect, AABB2 inRect) {
			NotImplemented();
		}
		
		void FrameDone() {
			NotImplemented();
		}
		void Flip() {
			NotImplemented();
		}
		
		Bitmap@ ReadBitmap() {
			NotImplemented();
			return null;
		}
		
		float ScreenWidth { 
			get {
				NotImplemented();
				return 0.f;
			}
		}
		float ScreenHeight { 
			get {
				NotImplemented();
				return 0.f;
			}
		}
		
		
	}
}
