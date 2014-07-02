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

#include "UIElement.h"

namespace spades { namespace editor {
	
#pragma mark - UIManager
	
	UIManager::UIManager(client::IRenderer *renderer):
	renderer(renderer) {
		SPAssert(renderer);
		
		defaultCursor.Set(renderer->RegisterImage("Gfx/UI/Cursor.png"), false);
		
		root.Set(new UIElement(this), false);
	}
	
	UIManager::~UIManager() {
		
	}
	
	void UIManager::Update(double dt) {
		time += dt;
	}
	
	void UIManager::Render() {
		root->Render();
		
		renderer->SetColorAlphaPremultiplied(Vector4(1,1,1,1));
		Cursor cursor;
		if (mouseFocus) {
			cursor = mouseFocus->GetCursor();
		}
		if (cursor.image == nullptr) {
			cursor.image = defaultCursor;
			cursor.hotSpot = Vector2(8, 8);
		}
		
		renderer->DrawImage(cursor.image,
							mousePos - cursor.hotSpot);
		
	}
	
	void UIManager::SetFont(client::IFont *f) {
		font.Set(f, true);
	}
	
	AABB2 UIManager::GetScreenBounds() {
		return AABB2(0, 0,
					 renderer->ScreenWidth(),
					 renderer->ScreenHeight());
	}
	
	client::IRenderer *UIManager::GetRenderer() {
		return renderer;
	}
	
	void UIManager::SetRootElement(UIElement *e) {
		root.Set(e, true);
	}
	
	void UIManager::SetMouseFocus(UIElement *e) {
		if (e == mouseFocus) return;
		auto *last = mouseFocus;
		mouseFocus = e;
		if (last) last->OnMouseLeave();
		if (mouseFocus) mouseFocus->OnMouseEnter();
	}
	
	void UIManager::SetKeyboardFocus(UIElement *e) {
		if (e == mouseFocus) return;
		auto *last = mouseFocus;
		mouseFocus = e;
		if (last) last->OnMouseLeave();
		if (mouseFocus) mouseFocus->OnMouseEnter();
	}
	
	void UIManager::MouseEvent(float x, float y) {
		mousePos = Vector2(x, y);
		if (pressed.empty())
			SetMouseFocus(root->HitTest(mousePos));
		if (mouseFocus) {
			mouseFocus->OnMouseMove(mousePos -
									mouseFocus->GetScreenBounds().min);
		}
	}
	
	void UIManager::KeyEvent(const std::string &key, bool down) {
		if (key == "Control") {
			if (down)
				modifiers.insert(KeyModifier::Control);
			else
				modifiers.erase(KeyModifier::Control);
		} else if (key == "Shift") {
			if (down)
				modifiers.insert(KeyModifier::Shift);
			else
				modifiers.erase(KeyModifier::Shift);
		} else if (key == "Alt") {
			if (down)
				modifiers.insert(KeyModifier::Alt);
			else
				modifiers.erase(KeyModifier::Alt);
		} else if (key == "Meta") {
			if (down)
				modifiers.insert(KeyModifier::Meta);
			else
				modifiers.erase(KeyModifier::Meta);
		}
		if (key == "LeftMouseButton" ||
			key == "RightMouseButton" ||
			key == "MiddleMouseButton") {
			MouseButton button;
			if (key == "LeftMouseButton")
				button = MouseButton::Left;
			else if (key == "RightMouseButton")
				button = MouseButton::Right;
			else if (key == "MiddleMouseButton")
				button = MouseButton::Middle;
			else
				SPRaise("unexpected");
			if (down) {
				pressed.insert(button);
			} else {
				pressed.erase(button);
			}
			if (mouseFocus) {
				if (down) {
					mouseFocus->OnMouseDown(button,
											mousePos -
											mouseFocus->GetScreenBounds().min);
				} else {
					mouseFocus->OnMouseUp(button,
											mousePos -
											mouseFocus->GetScreenBounds().min);
				}
			}
			if (pressed.empty()) {
				SetMouseFocus(root->HitTest(mousePos));
			}
		}
	}
	
	void UIManager::TextInputEvent(const std::string &text) {
		if (keyboardFocus)
			keyboardFocus->OnTextInputEvent(text);
	}
	
	void UIManager::TextEditingEvent(const std::string &text,
									 int start, int len) {
		if (keyboardFocus)
			keyboardFocus->OnTextEditingEvent(text, start, len);
	}
	
	bool UIManager::AcceptsTextInput() {
		return keyboardFocus;
	}
	
	AABB2 UIManager::GetTextInputRect() {
		if (keyboardFocus) {
			auto r = keyboardFocus->GetTextInputRect();
			return r.Translated(keyboardFocus->GetScreenBounds().min);
		}
		return AABB2(0, 0, 0, 0);
	}
	
	bool UIManager::NeedsAbsoluteMouseCoordinate() {
		return true;
	}
	
	void UIManager::WheelEvent(float x, float y) {
		if (mouseFocus) mouseFocus->OnMouseWheel(Vector2(x, y));
	}
	
	bool UIManager::GetKeyModifierState(KeyModifier mod) {
		return modifiers.find(mod) != modifiers.end();
	}
	
#pragma mark - UIElement
	
	UIElement::UIElement(UIManager *m):
	manager(m) {
		SPAssert(m);
		
	}
	
	UIElement::~UIElement() {
		PrepareToDetach();
		RemoveFromParent();
		for (const auto& e: children) {
			SPAssert(e->parent == this);
			e->parent = nullptr;
		}
	}
	
	void UIElement::PrepareToDetach() {
		if (manager->mouseFocus &&
			manager->mouseFocus->IsDescendantOf(this)) {
			manager->SetMouseFocus(nullptr);
		}
		if (manager->keyboardFocus &&
			manager->keyboardFocus->IsDescendantOf(this)) {
			manager->SetKeyboardFocus(nullptr);
		}
	}
	
	void UIElement::AddChildToFront(UIElement *e) {
		SPAssert(e);
		e->RemoveFromParent();
		auto it = children.emplace(children.end(),
								   e);
		e->childIterator = it;
		e->parent = this;
	}
	
	
	void UIElement::AddChildToBack(UIElement *e) {
		SPAssert(e);
		e->RemoveFromParent();
		auto it = children.emplace(children.begin(),
								   e);
		e->childIterator = it;
		e->parent = this;
	}
	
	void UIElement::RemoveFromParent() {
		if (!parent) return;
		PrepareToDetach();
		Handle<UIElement> e(this); // hold reference
		parent->children.erase(childIterator);
		parent = nullptr;
	}
	
	bool UIElement::IsDescendantOf(UIElement *e) {
		if (e == this) return this;
		if (parent) return parent->IsDescendantOf(e);
		return false;
	}
	
	bool UIElement::IsAncestorOf(UIElement *e) {
		if (!e) return false;
		return e->IsDescendantOf(this);
	}
	
	void UIElement::SetBounds(const AABB2 &b) {
		bounds = b;
	}
	
	AABB2 UIElement::GetScreenBounds() const {
		if (parent)
			return GetBounds().Translated(parent->GetScreenBounds().min);
		else
			return GetBounds();
	}
	
	AABB2 UIElement::GetLocalBounds() const {
		return AABB2(0, 0, bounds.GetWidth(),
					 bounds.GetHeight());
	}
	
	UIElement *UIElement::HitTest(const Vector2 & v) {
		if (!(bounds && v)) return nullptr;
		if (!IsElementEnabled()) return nullptr;
		auto vv = v - bounds.min;
		if (!HitTestLocal(vv)) return nullptr;
		for (auto it = children.rbegin();
			 it != children.rend();) {
			auto cur = it++;
			auto *r = (*cur)->HitTest(vv);
			if (r) return r;
		}
		return this;
	}
	
	bool UIElement::HitTestLocal(const spades::Vector2 &) { return true; }
	
	void UIElement::Render() {
		RenderClient();
		
		for (const auto& e: children) {
			e->Render();
		}
	}
	
	void UIElement::RenderClient() { }
	
	void UIElement::OnMouseEnter() { }
	void UIElement::OnMouseLeave() { }
	void UIElement::OnMouseDown(MouseButton b, const spades::Vector2 &) {
		if (IsEnabled() && AcceptsKeyboardFocus()) {
			manager->SetKeyboardFocus(this);
		} else {
			manager->SetKeyboardFocus(nullptr);
		}
	}
	void UIElement::OnMouseUp(MouseButton, const spades::Vector2 &) { }
	void UIElement::OnMouseMove(const spades::Vector2 &) { }
	
	void UIElement::OnTextInputEvent(const std::string &) { }
	void UIElement::OnTextEditingEvent(const std::string &, int, int) { }

	AABB2 UIElement::GetTextInputRect() {
		return AABB2(0,0,0,0);
	}
	
	bool UIElement::IsEnabled() {
		if (!enabled) return false;
		if (parent)
			return parent->IsEnabled();
		else
			return true;
	}
	
	
} }

