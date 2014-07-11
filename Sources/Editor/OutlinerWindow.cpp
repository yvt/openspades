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

#include "OutlinerWindow.h"
#include "Editor.h"
#include "ListView.h"
#include "TreeView.h"
#include "Buttons.h"

namespace spades { namespace editor {
	
	class TestTreeItem: public TreeViewItem {
		std::vector<Handle<TestTreeItem>> children;
	protected:
		~TestTreeItem() { }
	public:
		virtual std::size_t GetNumChildren() {
			return children.size();
		}
		virtual TreeViewItem *CreateChild(std::size_t index) {
			if (!children[index]) {
				children[index] = MakeHandle<TestTreeItem>();
			}
			return children[index];
		}
		virtual UIElement *CreateView(UIManager *m) {
			auto b = MakeHandle<Button>(m);
			char bf[256];
			sprintf(bf, "0x%08x", (int)this);
			b->SetText(bf);
			return b.Unmanage();
		}
		TestTreeItem() {
			children.resize((rand() & 7));
		}
	};
	
	OutlinerWindow::OutlinerWindow
	(UIManager *m, Editor&e):
	Window(m),
	editor(e) {
		listView = MakeHandle<ListView>(m);
		AddChildToFront(listView);
		
		auto model = MakeHandle<TreeViewModel>
		(m, MakeHandle<TestTreeItem>());
		listView->SetModel(model);
	}
	
	OutlinerWindow::~OutlinerWindow() { }
	
	client::IFont *OutlinerWindow::GetTitleFont() {
		return editor.GetTitleFont();
	}
	
	std::string OutlinerWindow::GetTitle() {
		return "Outline";
	}
	
	void OutlinerWindow::RenderClient() {
		Window::RenderClient();
		listView->SetBounds(GetClientBounds());
	}
	
	Vector2 OutlinerWindow::AdjustClientSize(const Vector2& sz) {
		auto rh = listView->GetRowHeight();
		auto sz2 = Vector2(sz.x, std::max(floorf(sz.y / rh + .5f), 3.f) * rh);
		return Window::AdjustClientSize(sz2);
	}
	
	
} }

