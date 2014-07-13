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

#include "ListView.h"
#include <list>
#include <Core/TMPUtils.h>

namespace spades { namespace editor {

	class TreeViewModel;
	
	class TreeViewItem: public RefCountedObject {
		friend class TreeViewModel;
		std::list<Handle<TreeViewItem>> children;
		bool childrenLoaded = false;
		std::set<TreeViewModel *> models;
		decltype(children)::iterator selfIter;
		
		TreeViewItem *parent = nullptr;
		
		std::size_t numChildrenRows;
		
		bool expanded = false;
		
		template <class F>
		void ForEachModel(F);
		
		std::size_t GetNumRows();
		std::size_t ComputeChildrenRows();
		
		stmp::optional<std::size_t> GetRowIndex();
		TreeViewItem *FindItemAtRow(std::size_t);
		
		void RecycleChild(TreeViewItem *);
		
	protected:
		~TreeViewItem();
		
		void ChildAdded(std::size_t index,
						std::size_t count);
		void ChildRemoved(std::size_t index,
						std::size_t count);
		
	public:
		TreeViewItem();
		
		bool IsExpanded() const { return expanded; }
		void SetExpanded(bool);
		
		virtual std::size_t GetNumChildren() { return 0; }
		virtual TreeViewItem *CreateChild(std::size_t) { return nullptr; };
		virtual UIElement *CreateView(UIManager *) = 0;
		virtual void RecycleView(UIElement *) { }
		
		TreeViewItem *GetParent() const { return parent; }
	};
	
	class TreeViewModel: public ListViewModel {
		friend class TreeViewItem;
		Handle<UIManager> manager;
		Handle<TreeViewItem> root;
	protected:
		~TreeViewModel();
	public:
		TreeViewModel(UIManager *, TreeViewItem *);
		std::size_t GetNumItems() override;
		UIElement *CreateItem(std::size_t) override;
		void RecycleItem(UIElement *) override;
	};

} }
