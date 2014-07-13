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

#include <list>
#include <Core/RefCountedObject.h>
#include <Client/IRenderer.h>
#include <set>
#include <Client/IFont.h>

namespace spades { namespace editor {
	
	class UIElement;
	
	enum class MouseButton {
		Left = 0,
		Right,
		Middle
	};
	
	enum class KeyModifier {
		Shift,
		Control,
		Alt,
		Meta
	};
	
	class UIManager: public RefCountedObject {
		friend class UIElement;
		
		Handle<client::IRenderer> renderer;
		Handle<UIElement> root;
		Handle<client::IFont> font;
		Handle<client::IImage> defaultCursor;
		double time = 0.;
		
		UIElement *mouseFocus = nullptr;
		UIElement *keyboardFocus = nullptr;
		Vector2 mousePos;
		std::set<MouseButton> pressed;
		bool shiftPressed;
		
		std::string keyRepeatKey;
		double keyRepeatTime = 0.;
		
		std::set<KeyModifier> modifiers;
		
		UIElement *GetKeyHandler();
		
	public:
		UIManager(client::IRenderer *);
		~UIManager();
		
		double GetTime() const { return time; }
		
		client::IFont *GetFont() { return font; }
		void SetFont(client::IFont *);
		
		void SetMouseFocus(UIElement *);
		void SetKeyboardFocus(UIElement *);
		
		Vector2 GetMousePosition() const { return mousePos; }
		
		void SetRootElement(UIElement *);
		
		void MouseEvent(float x, float y);
		void KeyEvent(const std::string&,
					  bool down);
		void TextInputEvent(const std::string&);
		void TextEditingEvent(const std::string&,
							  int start, int len);
		bool AcceptsTextInput();
		AABB2 GetTextInputRect();
		bool NeedsAbsoluteMouseCoordinate();
		void WheelEvent(float x, float y);
		
		bool GetKeyModifierState(KeyModifier);
		
		client::IRenderer *GetRenderer();
		
		void Update(double dt);
		void Render();
		AABB2 GetScreenBounds();
	};
	
	struct Cursor {
		client::IImage *image = nullptr;
		Vector2 hotSpot { 0, 0 };
	};
	
	class UIElement: public RefCountedObject {
		friend class UIManager;
		
		UIManager *const manager;
		UIElement *parent = nullptr;
		std::list<Handle<UIElement>> children;
		decltype(children)::iterator childIterator;
		AABB2 bounds;
		
		bool enabled = true;
		
		void PrepareToDetach();
	protected:
		~UIElement();
		
		virtual void RenderClient();
		virtual bool HitTestLocal(const Vector2&);
		virtual void OnMouseEnter();
		virtual void OnMouseLeave();
		virtual void OnEnter() { }
		virtual void OnLeave() { }
		virtual void OnMouseDown(MouseButton, const Vector2&);
		virtual void OnMouseUp(MouseButton, const Vector2&);
		virtual void OnMouseMove(const Vector2&);
		virtual void OnMouseWheel(const Vector2&) { }
		virtual void OnKeyDown(const std::string&) { }
		virtual void OnKeyUp(const std::string&) { }
		virtual void OnTextInputEvent(const std::string&);
		virtual void OnTextEditingEvent(const std::string&,
							  int start, int len);
	public:
		UIElement(UIManager *);
		
		void AddChildToFront(UIElement *);
		void AddChildToBack(UIElement *);
		
		UIManager& GetManager() const { return *manager; }
		
		client::IFont *GetFont();
		
		void RemoveFromParent();
		
		const decltype(children)& GetChildren() const
		{ return children; }
		UIElement *GetParent() const { return parent; }
		
		bool IsDescendantOf(UIElement *);
		bool IsAncestorOf(UIElement *);
		
		AABB2 GetBounds() const { return bounds; }
		void SetBounds(const AABB2&);
		
		AABB2 GetScreenBounds() const;
		AABB2 GetLocalBounds() const;
		
		UIElement *HitTest(const Vector2&);
		
		bool IsEnabled();
		bool IsElementEnabled() const { return enabled; }
		void SetEnabled(bool b) { enabled = b; }
		
		virtual bool AcceptsKeyboardFocus() { return false; }
		virtual AABB2 GetTextInputRect();
		virtual Cursor GetCursor() { return Cursor(); }
		
		bool HasMouseFocus();
		bool HasKeyboardFocus();
		
		void Render();
	};
	
	class Label: public UIElement {
		std::string text;
		Vector2 alignment {0, 0};
		Vector4 color { 0, 0, 0, 1 };
		Vector4 shadowColor { 0, 0, 0, 0 };
	protected:
		~Label();
		void RenderClient() override;
	public:
		Label(UIManager *);
		
		void SetText(const std::string& s) { text = s; }
		std::string GetText() const { return text; }
		void SetAlignment(const Vector2& v) { alignment = v; }
		Vector2 GetAlignment() const { return alignment; }
		void SetColor(const Vector4& c) { color = c; }
		Vector4 GetColor() const { return color; }
		void SetShadowColor(const Vector4& c) { shadowColor = c; }
		Vector4 GetShadowColor() const { return shadowColor; }
	};
	
} }
