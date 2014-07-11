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

#include "UIElement.h"

#include <list>
#include <set>

namespace spades { namespace editor {

	class ListView;
	class ScrollBar;
	
	class ListViewModel: public RefCountedObject {
		friend class ListView;
		std::set<ListView *> listViews;
	protected:
		~ListViewModel() { }
		void ItemAdded(std::size_t index,
					   std::size_t count);
		void ItemRemoved(std::size_t index,
						 std::size_t count);
		void ItemMoved(std::size_t index,
					   std::size_t count,
					   std::size_t newIndex);
	public:
		ListViewModel() { }
		virtual std::size_t GetNumItems() = 0;
		virtual UIElement *CreateItem(std::size_t) = 0;
		virtual void RecycleItem(UIElement *) {}
	};
	
	class ListView: public UIElement {
		friend class ListViewModel;
		Handle<ScrollBar> scrollBar;
		
		Handle<ListViewModel> model;
		float rowHeight = 20.f;
		
		std::size_t loadedIndex = 0;
		std::list<Handle<UIElement>> loadedItems;
		
		std::size_t GetNumVisibleRows();
		
		void Recycle(UIElement *);
		
		void ItemAdded(std::size_t index,
					   std::size_t count);
		void ItemRemoved(std::size_t index,
						 std::size_t count);
		void ItemMoved(std::size_t index,
					   std::size_t count,
					   std::size_t newIndex);
		
		void Layout();
	protected:
		~ListView();
		void RenderClient() override;
	public:
		ListView(UIManager *);
		
		void SetRowHeight(float);
		float GetRowHeight() const { return rowHeight; }
		
		void SetModel(ListViewModel *);
		ListViewModel *GetModel() const { return model; }
		
		
	};

} }
