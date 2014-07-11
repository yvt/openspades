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
#include "ListView.h"
#include "ScrollBar.h"

namespace spades { namespace editor {

	ListView::ListView(UIManager *m):
	UIElement(m) {
		scrollBar = MakeHandle<ScrollBar>(m);
		AddChildToFront(scrollBar);
		
		scrollBar->SetChangeHandler([&] {
			Layout();
		});
	}
	
	ListView::~ListView() {
		if (model)
			model->listViews.erase(this);
	}
	
	void ListView::SetModel(ListViewModel *model) {
		if (this->model == model) return;
		if (model) model->listViews.erase(this);
		for (const auto& item: loadedItems) {
			Recycle(item);
		}
		loadedItems.clear();
		this->model = ToHandle(model);
		if (model) model->listViews.insert(this);
	}
	
	void ListView::SetRowHeight(float h) {
		rowHeight = h;
	}
	
	std::size_t ListView::GetNumVisibleRows() {
		return static_cast<std::size_t>
		(GetBounds().GetHeight() / rowHeight);
	}
	
	void ListView::Layout() {
		auto sz = GetBounds().GetSize();
		scrollBar->SetBounds(AABB2(sz.x - 16.f, 0, 16.f, sz.y));
		
		auto numVisibleRows = GetNumVisibleRows();
		auto numRows = model ? model->GetNumItems() : 0;
		auto visibleStart =
		static_cast<std::size_t>(std::max
								 (0., scrollBar->GetValue()+.5));
		auto visibleEnd = visibleStart + numVisibleRows;
		visibleStart = std::min(visibleStart, numRows);
		visibleEnd = std::min(visibleEnd, numRows);
		
		auto loadedStart = loadedIndex;
		auto loadedEnd = loadedIndex + loadedItems.size();
		
		if (loadedItems.empty() ||
			loadedStart >= visibleEnd ||
			loadedEnd <= visibleStart) {
			for (const auto& item: loadedItems) {
				Recycle(item);
			}
			loadedItems.clear();
			for (std::size_t i = visibleStart;
				 i < visibleEnd; ++i) {
				loadedItems.push_back(nullptr);
			}
			loadedStart = visibleStart;
			loadedEnd = visibleEnd;
		}
		
		while (loadedStart < visibleStart) {
			auto item = loadedItems.front();
			Recycle(item);
			loadedItems.pop_front();
			++loadedStart;
		}
		while (loadedEnd > visibleEnd) {
			auto item = loadedItems.back();
			Recycle(item);
			loadedItems.pop_back();
			--loadedEnd;
		}
		while (loadedStart > visibleStart) {
			--loadedStart;
			loadedItems.push_front(nullptr);
		}
		while (loadedEnd < visibleEnd) {
			loadedItems.push_back(nullptr);
			++loadedEnd;
		}
		
		SPAssert(loadedStart == visibleStart);
		SPAssert(loadedEnd == visibleEnd);
		
		loadedIndex = loadedStart;
		
		if (numVisibleRows >= numRows) {
			scrollBar->SetEnabled(false);
			scrollBar->SetRange(0, 0);
		} else {
			scrollBar->SetEnabled(true);
			scrollBar->SetRange(0, numRows - numVisibleRows);
			scrollBar->SetSmallChange(1);
			scrollBar->SetLargeChange(numVisibleRows);
		}
		
		std::size_t index = loadedIndex;
		for (auto& item: loadedItems) {
			if (!item) {
				auto it = ToHandle(model->CreateItem(index));
				SPAssert(it);
				AddChildToFront(it);
				item = it;
			}
			++index;
		}
		
		float y = 0.f;
		for (const auto& item: loadedItems) {
			item->SetBounds(AABB2(0.f, y, sz.x - 16.f, rowHeight));
			y += rowHeight;
		}
	}
	
	void ListView::RenderClient() {
		Layout();
		UIElement::RenderClient();
	}
	
	void ListView::Recycle(UIElement *e) {
		if (e) {
			e->RemoveFromParent();
			model->RecycleItem(e);
		}
	}
	
	void ListViewModel::ItemAdded(std::size_t index,
								  std::size_t count) {
		for (auto *lv: listViews)
			lv->ItemAdded(index, count);
	}
	
	void ListViewModel::ItemRemoved(std::size_t index,
									std::size_t count) {
		for (auto *lv: listViews)
			lv->ItemRemoved(index, count);
	}
	
	void ListViewModel::ItemMoved(std::size_t index,
								  std::size_t count,
								  std::size_t newIndex) {
		for (auto *lv: listViews)
			lv->ItemMoved(index, count, newIndex);
	}
	
	void ListView::ItemAdded(std::size_t index,
							 std::size_t count) {
		
		auto loadedStart = loadedIndex;
		auto loadedEnd = loadedIndex + loadedItems.size();
		
		if (index <= loadedStart) {
			loadedIndex += count;
		} else if (index < loadedEnd) {
			auto it = loadedItems.begin();
			std::advance(it, index - loadedStart);
			auto pushCount = std::min(loadedEnd - index, count);
			while (pushCount--) {
				loadedItems.insert(it, nullptr);
				Recycle(loadedItems.back());
				loadedItems.pop_back();
			}
		}
		
	}
	
	void ListView::ItemRemoved(std::size_t index,
							   std::size_t count) {
		
		auto loadedStart = loadedIndex;
		auto loadedEnd = loadedIndex + loadedItems.size();
		
		auto indexEnd = index + count;
		
		if (indexEnd <= loadedStart) {
			loadedIndex -= count;
		} else if (index <= loadedEnd) {
			if (index <= loadedStart) {
				indexEnd = std::min(loadedEnd, indexEnd);
				while (loadedStart < indexEnd) {
					auto item = loadedItems.front();
					Recycle(item);
					loadedItems.pop_front();
					++loadedStart;
				}
				loadedIndex = indexEnd;
			} else if (indexEnd >= loadedEnd) {
				while (loadedEnd > index) {
					auto item = loadedItems.back();
					Recycle(item);
					loadedItems.pop_back();
					--loadedEnd;
				}
			} else {
				auto it1 = loadedItems.begin();
				auto it2 = it1;
				std::advance(it1, index - loadedStart);
				std::advance(it2, indexEnd - loadedStart);
				for (auto it = it1; it != it2; ++it) {
					Recycle(*it);
				}
				loadedItems.erase(it1, it2);
			}
		}
		
	}

	void ListView::ItemMoved(std::size_t index,
							 std::size_t count,
							 std::size_t newIndex) {
		if (newIndex >= index) {
			if (newIndex <= index + count) {
				newIndex = index;
			} else {
				newIndex -= count;
			}
		}
		
		ItemRemoved(index, count);
		ItemAdded(newIndex, count);
	}
	
} }
