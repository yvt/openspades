#include "/Base/IImage.as"

namespace spades {
	class NativeImage: IImage {
		private LowLevelNativeImage@ inner;
		NativeImage(LowLevelNativeImage@ inner) {
			@this.inner = inner;
		}
		float Width {
			get {
				return inner.Width;
			}
		}
		float Height {
			get {
				return inner.Height;
			}
		}
	};
}