#include "/Base/IModel.as"

namespace spades {
	class NativeModel: IModel {
		private LowLevelNativeModel@ inner;
		NativeModel(LowLevelNativeModel@ inner){
			@this.inner = inner;
		}
	}	
}
