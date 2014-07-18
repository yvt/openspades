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

#include "ObjectMode.h"
#include "UIElement.h"
#include "Editor.h"
#include "MainView.h"
#include "Scene.h"

namespace spades { namespace editor {
	
	ObjectSelectionMode::ObjectSelectionMode() { }
	ObjectSelectionMode::~ObjectSelectionMode() { }
	
	void ObjectSelectionMode::Enter(Editor *) { }
	void ObjectSelectionMode::Leave(Editor *) { }
	
	class ObjectSelectionView: public ModeView {
		Editor& editor;
	protected:
		void OnMouseDown(MouseButton mb, const Vector2& v) override {
			if (mb == MouseButton::Left) {
				auto dir = editor.Unproject(v);
				auto *sc = editor.GetScene();
				if (!sc) return;
				
				auto ret = sc->CastRay(editor.GetLastSceneDefinition().viewOrigin,
									   dir, editor.GetPose());
				
				if (ret) {
					auto& r = *ret;
					auto shift = GetManager().GetKeyModifierState(KeyModifier::Shift);
					if (shift && editor.IsSelected(r.frame)) {
						editor.Deselect(r.frame);
					} else {
						editor.Select(r.frame,
									  shift);
					}
				}
				
				return;
			}
			ModeView::OnMouseDown(mb, v);
		}
	public:
		ObjectSelectionView(UIManager *m, Editor *e):
		ModeView(m, e), editor(*e) {
		}
		
		
	};
	
	
	UIElement *ObjectSelectionMode::CreateView
	(UIManager *m, Editor *e) {
		return new ObjectSelectionView(m, e);
	}
	
} }
