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

#include "TreeView.h"
#include <limits>
#include "Buttons.h"
#include <Client/IRenderer.h>
#include <Client/IImage.h>

namespace spades { namespace editor {
	
	class TreeViewItemExpander: public ButtonBase {
		Handle<client::IImage> img;
		Handle<TreeViewItem> item;
	protected:
		void RenderClient() override {
			auto *r = GetManager().GetRenderer();
			auto rt = GetScreenBounds();
			
			auto state = GetState();
			switch (state) {
				case ButtonState::Default:
					r->SetColorAlphaPremultiplied(Vector4(1,1,1,1));
					break;
				case ButtonState::Hover:
					r->SetColorAlphaPremultiplied(Vector4(1.2,1.2,1.2,1));
					break;
				case ButtonState::Pressed:
					r->SetColorAlphaPremultiplied(Vector4(.95,.95,.95,1));
					break;
			}
			
			auto p = (rt.min + rt.max - Vector2(16, 16)) * .5f;
			p = (p + .5f).Floor();
			r->DrawImage(img, AABB2(p.x, p.y, 16, 16),
						 AABB2(item->IsExpanded() ? 16 : 0, 0,
							   16, 16));
		}
		~TreeViewItemExpander() {
			
		}
	public:
		TreeViewItemExpander(UIManager *m,
							 TreeViewItem *item):
		ButtonBase(m), item(item) {
			img = ToHandle(m->GetRenderer()->RegisterImage
						   ("Gfx/UI/TreeIcon.png"));
			SetActivateHandler([&] {
				this->item->SetExpanded(!this->item->IsExpanded());
			});
		}
	};
	
	class TreeViewItemElement: public UIElement {
		Handle<TreeViewItemExpander> ex;
		Handle<UIElement> inner;
		Handle<TreeViewItem> item;
		float indent;
		void Layout() {
			auto sz = GetBounds().GetSize();
			ex->SetBounds(AABB2(indent, 0, 16, sz.y));
			inner->SetBounds(AABB2(indent + 16.f, 0,
								   sz.x - indent - 16.f,
								   sz.y));
		}
	protected:
		~TreeViewItemElement() { }
		void RenderClient() override {
			Layout();
			UIElement::RenderClient();
		}
	public:
		TreeViewItemElement(UIManager *m,
							UIElement *e,
							TreeViewItem *item,
							int level):
		UIElement(m),
		inner(e, true),
		indent(level * 16.f),
		item(item) {
			SPAssert(e);
			SPAssert(item);
			SPAssert(m);
			ex = MakeHandle<TreeViewItemExpander>
			(m, item);
			
			AddChildToFront(ex);
			AddChildToFront(inner);
		}
		
		void Recycle() {
			item->RecycleView(this);
		}
		
	};
	
	TreeViewItem::TreeViewItem():
	numChildrenRows(0) { }

	TreeViewItem::~TreeViewItem() {
		for (const auto& c: children) {
			RecycleChild(c);
		}
	}
	
	void TreeViewItem::RecycleChild(TreeViewItem *i) {
		if (i) {
			SPAssert(i->parent == this);
			i->parent = nullptr;
		}
	}
	
	std::size_t TreeViewItem::GetNumRows() {
		return 1 + (expanded ? numChildrenRows : 0);
	}
	
	std::size_t TreeViewItem::ComputeChildrenRows() {
		std::size_t total = 0;
		for (const auto& ch: children) {
			SPAssert(ch);
			total += ch->GetNumRows();
		}
		return total;
	}

	template <class F>
	void TreeViewItem::ForEachModel(F functor) {
		if (!models.empty()) {
			for (auto *m: models) {
				functor(*this, *m);
			}
		}
		if (parent)
			parent->ForEachModel(functor);
	}
	
	stmp::optional<std::size_t> TreeViewItem::GetRowIndex() {
		if (!parent) return 0;
		if (!parent->expanded) return stmp::optional<std::size_t>();
		
		auto pindex = parent->GetRowIndex();
		if (!pindex) return pindex;
		
		auto firstSib = parent->children.begin();
		std::size_t i = 0;
		auto it = selfIter;
		while (it != firstSib) {
			--it;
			i += (*it)->GetNumRows();
		}
		
		return *pindex + i + 1;
	}
	
	TreeViewItem *TreeViewItem::FindItemAtRow(std::size_t i) {
		if (i == 0) return this;
		if (!expanded) return nullptr;
		std::size_t r = 1;
		for (const auto& c: children) {
			SPAssert(c);
			r += c->GetNumRows();
			if (r > i)
				return c->FindItemAtRow(i - (r - c->GetNumRows()));
		}
		return nullptr;
	}
	
	void TreeViewItem::SetExpanded(bool b) {
		if (b == expanded) return;
		auto oldRows = GetNumRows();
		auto updateParentRows = [&]() {
			std::ptrdiff_t diff = GetNumRows() - oldRows;
			auto *p = parent;
			while (p) {
				p->numChildrenRows += diff;
				p = p->parent;
			}
		};
		expanded = b;
		if (b) {
			if (!childrenLoaded) {
				auto num = GetNumChildren();
				for (std::size_t i = 0; i < num; ++i) {
					auto *e = CreateChild(i);
					SPAssert(e->parent == nullptr);
					e->parent = this;
					children.push_back(ToHandle(e));
					e->selfIter = children.end();
					--e->selfIter;
				}
				numChildrenRows = ComputeChildrenRows();
				childrenLoaded = true;
			}
			
			// create items for children
			std::size_t i = 0;
			for (auto it = children.begin();
				 it != children.end(); ++it) {
				auto& c = *it;
				if (!c) {
					auto *e = CreateChild(i);
					e->selfIter = it;
					SPAssert(e->parent == nullptr);
					e->parent = this;
					c.Set(e, false);
				}
				++i;
			}
			
			updateParentRows();
			
			auto idx = GetRowIndex();
			if (idx) {
				ForEachModel([&](TreeViewItem& root,
								 TreeViewModel& model) {
					auto ridx = root.GetRowIndex();
					SPAssert(ridx);
					model.ItemAdded(*idx - *ridx + 1, numChildrenRows);
				});
			}
		} else {
			updateParentRows();
			
			auto idx = GetRowIndex();
			if (idx) {
				ForEachModel([&](TreeViewItem& root,
								 TreeViewModel& model) {
					auto ridx = root.GetRowIndex();
					SPAssert(ridx);
					model.ItemRemoved(*idx - *ridx + 1, numChildrenRows);
				});
			}
		}
	}
	
	void TreeViewItem::ChildAdded(std::size_t index,
								  std::size_t count) {
		if (!childrenLoaded) return;
		if (count == 0) return;
		SPAssert(index + count <= children.size());
		
		auto it = children.begin();
		std::advance(it, index);
		
		while(count--) {
			if (expanded) {
				auto *e = CreateChild(index);
				e->selfIter = it;
				SPAssert(e->parent == nullptr);
				e->parent = this;
				children.insert(it, ToHandle(e));
			} else {
				children.insert(it, nullptr);
			}
			++index; ++it;
		}
		
		if (expanded) {
			children.begin();
			std::advance(it, index);
			auto startRow = (*it)->GetRowIndex();
			SPAssert(startRow);
			ForEachModel([&](TreeViewItem& root,
							 TreeViewModel& model) {
				auto ridx = root.GetRowIndex();
				SPAssert(ridx);
				model.ItemAdded(*ridx - *startRow + index + 1,
								count);
			});
		}
		
	}
	
	void TreeViewItem::ChildRemoved(std::size_t index,
									std::size_t count) {
		if (!childrenLoaded) return;
		if (count == 0) return;
		
		SPAssert(index + count <= children.size());
		
		auto it1 = children.begin();
		std::advance(it1, index);
		auto it2 = it1;
		std::advance(it2, count);
		
		auto startIndex = (*it1)->GetRowIndex();
		auto it3 = it2; --it3;
		auto endIndex = (*it3)->GetRowIndex();
		if (endIndex) {
			*endIndex += (*it3)->GetNumRows();
		}
		
		for (auto it = it1; it != it2; ++it) {
			RecycleChild(*it);
		}
		
		children.erase(it1, it2);
		
		if (expanded) {
			SPAssert(startIndex);
			SPAssert(endIndex);
			ForEachModel([&](TreeViewItem& root,
							 TreeViewModel& model) {
				auto ridx = root.GetRowIndex();
				SPAssert(ridx);
				model.ItemAdded(*startIndex - *ridx,
								*endIndex - *startIndex);
			});
		}
	}

	TreeViewModel::TreeViewModel(UIManager *m,
								 TreeViewItem *root):
	manager(m), root(root) {
		SPAssert(m);
		SPAssert(root);
		root->models.insert(this);
	}
	
	TreeViewModel::~TreeViewModel() {
		root->models.erase(this);
	}
	
	std::size_t TreeViewModel::GetNumItems() {
		return root->GetNumRows();
	}
	
	UIElement *TreeViewModel::CreateItem(std::size_t row) {
		auto *item = root->FindItemAtRow(row);
		SPAssert(item);
		
		int level = 0;
		for (auto *t = item; t && t != root;) {
			level++;
			t = t->GetParent();
		}
		
		auto inner = ToHandle(item->CreateView(manager));
		SPAssert(inner);
		
		auto elm = MakeHandle<TreeViewItemElement>
		(manager, inner, item, level);
		
		return elm.Unmanage();
	}
	
	void TreeViewModel::RecycleItem(UIElement *item) {
		auto *e = dynamic_cast<TreeViewItemElement *>(item);
		SPAssert(e);
		e->Recycle();
	}
	
	
} }
