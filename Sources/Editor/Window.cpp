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

#include "Window.h"
#include "Buttons.h"

namespace spades { namespace editor {
	
	class Window::CloseButton: public ButtonBase {
		Handle<client::IImage> img;
	protected:
		void RenderClient() override {
			auto r = ToHandle(GetManager().GetRenderer());
			auto rt = GetScreenBounds();
			
			switch (GetState()) {
				case ButtonState::Default:
					r->SetColorAlphaPremultiplied(Vector4(0,0,0,0.4f));
					break;
				case ButtonState::Hover:
					r->SetColorAlphaPremultiplied(Vector4(0,0,0,0.5f));
					break;
				case ButtonState::Pressed:
					r->SetColorAlphaPremultiplied(Vector4(0,0,0,0.6f));
					break;
			}
			
			r->DrawImage(img, (rt.GetMid() -
						 Vector2(img->GetWidth(),
								 img->GetHeight() + 2.f) * .5f)
						 .Floor());
			
			ButtonBase::RenderClient();
		}
		~CloseButton() { }
	public:
		CloseButton(UIManager *m):
		ButtonBase(m) {
			img = ToHandle(GetManager().GetRenderer()
						   ->RegisterImage("Gfx/UI/Close.png"));
		}
	};
	
	class Window::DragHandle {
	public:
		virtual void DragStart() = 0;
		virtual void DragMove(const Vector2&) = 0;
	};
	
	struct Window::DragHandles {
		class Grab: public DragHandle {
			Window& w;
			Vector2 startPos;
		public:
			Grab(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds().min;
			}
			void DragMove(const Vector2& p) override {
				auto newbnd = w.GetBounds();
				newbnd = newbnd.Translated(startPos + p - newbnd.min);
				
				auto scrSize = w.GetManager().GetScreenBounds().GetSize();
				if (newbnd.min.x < 0.)
					newbnd = newbnd.Translated(-newbnd.min.x, 0);
				if (newbnd.min.y < 0.)
					newbnd = newbnd.Translated(0, -newbnd.min.y);
				
				if (newbnd.max.x > scrSize.x)
					newbnd = newbnd.Translated(scrSize.x-newbnd.max.x, 0);
				if (newbnd.max.y > scrSize.y)
					newbnd = newbnd.Translated(0, scrSize.y-newbnd.max.y);
				w.SetBounds(newbnd);
			}
		};
		class Left: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			Left(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto newMin = Vector2(std::max(startPos.min.x + p.x, 0.f),
									  startPos.min.y);
				auto newSize = startPos.max - newMin;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(startPos.max - newSize, startPos.max);
				
				w.SetBounds(newBnd);
			}
		};
		class Top: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			Top(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto newMin = Vector2(startPos.min.x,
									  std::max(startPos.min.y + p.y, 0.f));
				auto newSize = startPos.max - newMin;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(startPos.max - newSize, startPos.max);
				
				w.SetBounds(newBnd);
			}
		};
		class Right: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			Right(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto scrSize = w.GetManager().GetScreenBounds().GetSize();
				auto newMax = Vector2(std::min(startPos.max.x + p.x, scrSize.x),
									  startPos.max.y);
				auto newSize = newMax - startPos.min;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(startPos.min, startPos.min + newSize);
				
				w.SetBounds(newBnd);
			}
		};
		class Bottom: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			Bottom(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto scrSize = w.GetManager().GetScreenBounds().GetSize();
				auto newMax = Vector2(startPos.max.x,
									  std::min(startPos.max.y + p.y, scrSize.y));
				auto newSize = newMax - startPos.min;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(startPos.min, startPos.min + newSize);
				
				w.SetBounds(newBnd);
			}
		};
		class TopLeft: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			TopLeft(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto newMin = Vector2(std::max(startPos.min.x + p.x, 0.f),
									  std::max(startPos.min.y + p.y, 0.f));
				auto newSize = startPos.max - newMin;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(startPos.max - newSize, startPos.max);
				
				w.SetBounds(newBnd);
			}
		};
		class TopRight: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			TopRight(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto scrSize = w.GetManager().GetScreenBounds().GetSize();
				auto newMin = Vector2(startPos.min.x,
									  std::max(startPos.min.y + p.y, 0.f));
				auto newMax = Vector2(std::min(startPos.max.x + p.x, scrSize.x),
									  startPos.max.y);
				auto newSize = newMax - newMin;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(newMin.x,
									newMax.y - newSize.y,
									newSize.x, newSize.y);
				
				w.SetBounds(newBnd);
			}
		};
		class BottomLeft: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			BottomLeft(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto scrSize = w.GetManager().GetScreenBounds().GetSize();
				auto newMin = Vector2(std::max(startPos.min.x + p.x, 0.f),
									  startPos.min.y);
				auto newMax = Vector2(startPos.max.x,
									  std::min(startPos.max.y + p.y, scrSize.y));
				auto newSize = newMax - newMin;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(newMax.x - newSize.x,
									newMin.y,
									newSize.x, newSize.y);
				
				w.SetBounds(newBnd);
			}
		};
		class BottomRight: public DragHandle {
			Window& w;
			AABB2 startPos;
		public:
			BottomRight(Window& w): w(w) { }
			void DragStart() override {
				startPos = w.GetBounds();
			}
			void DragMove(const Vector2& p) override {
				auto scrSize = w.GetManager().GetScreenBounds().GetSize();
				auto newMax = Vector2(std::min(startPos.max.x + p.x, scrSize.x),
									  std::min(startPos.max.y + p.y, scrSize.y));
				auto newSize = newMax - startPos.min;
				newSize = w.AdjustSize(newSize);
				
				auto newBnd = AABB2(startPos.min, startPos.min + newSize);
				
				w.SetBounds(newBnd);
			}
		};
	};
	
	class Window::Mover: public UIElement {
		Window& w;
		std::shared_ptr<DragHandle> hnd;
		Vector2 startPos { 0, 0 };
		bool drag = false;
	protected:
		void OnMouseDown(MouseButton b, const Vector2&) override {
			if (b == MouseButton::Left) {
				hnd->DragStart();
				drag = true;
				startPos = GetManager().GetMousePosition();
			}
		}
		void OnMouseUp(MouseButton b, const Vector2&) override {
			if (b == MouseButton::Left) {
				drag = false;
			}
		}
		void OnMouseMove(const Vector2&) override {
			if (drag) {
				hnd->DragMove(GetManager().GetMousePosition() - startPos);
			}
		}
		~Mover() { }
	public:
		Mover(UIManager *m, Window& w,
			  std::shared_ptr<DragHandle> hnd):
		UIElement(m), w(w), hnd(hnd) { }
	};
	
	Window::Window(UIManager *m):
	UIElement(m) {
		moverGrab = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::Grab>(*this));
		AddChildToFront(moverGrab);
		moverLeft = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::Left>(*this));
		AddChildToFront(moverLeft);
		moverRight = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::Right>(*this));
		AddChildToFront(moverRight);
		moverTop = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::Top>(*this));
		AddChildToFront(moverTop);
		moverBottom = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::Bottom>(*this));
		AddChildToFront(moverBottom);
		
		moverTopLeft = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::TopLeft>(*this));
		AddChildToFront(moverTopLeft);
		moverTopRight = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::TopRight>(*this));
		AddChildToFront(moverTopRight);
		
		moverBottomLeft = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::BottomLeft>(*this));
		AddChildToFront(moverBottomLeft);
		moverBottomRight = MakeHandle<Mover>
		(m, *this, std::make_shared<DragHandles::BottomRight>(*this));
		AddChildToFront(moverBottomRight);
		
		closeButton = MakeHandle<CloseButton>
		(m);
		AddChildToFront(closeButton);
		closeButton->SetActivateHandler([&] {
			OnClose();
		});
	}
	
	Window::~Window() {
		
	}
	
	std::string Window::GetTitle() {
		return "Untitled";
	}
	
	void Window::OnClose() {
		RemoveFromParent();
	}
	
	client::IFont *Window::GetTitleFont() {
		return GetFont();
	}
	
	AABB2 Window::GetClientBounds() {
		auto sz = GetLocalBounds().GetSize();
		auto mg = 4.f;
		return AABB2(mg, 15.f+mg, sz.x - mg*2,
					 sz.y - 15.f - mg*2);
	}
	
	Vector2 Window::AdjustClientSize(const spades::Vector2 &v) {
		auto sz = v;
		sz.x = std::max(sz.x, 4.f);
		sz.y = std::max(sz.y, 4.f);
		sz.x = std::max(sz.x, GetTitleFont()->Measure(GetTitle()).x
						+ 16.f);
		return sz;
	}
	
	Vector2 Window::AdjustSize(const spades::Vector2 &v) {
		auto diff = GetBounds().GetSize() - GetClientBounds().GetSize();
		return AdjustClientSize(v - diff) + diff;
	}
	
	void Window::RenderClient() {
		Layout();
		
		auto rt = GetScreenBounds();
		auto r = ToHandle(GetManager().GetRenderer());
		
		r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.1f);
		r->DrawImage(nullptr, rt.Inflate(1));
		
		r->SetColorAlphaPremultiplied(Vector4(.95,.95,.95,1));
		r->DrawImage(nullptr, rt);
		
		auto f = ToHandle(GetTitleFont());
		auto ttl = GetTitle();
		auto sz = f->Measure(ttl);
		auto cli = GetClientBounds();
		auto mg = cli.min.x;
		auto ttlSize = cli.min.y;
		f->Draw(ttl,
				rt.min + Vector2(mg, (ttlSize - sz.y - 1) * .5f),
				1.f, Vector4(.3,.3,.3,1));
		
		r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.2f);
		r->DrawImage(nullptr,
					 AABB2(mg + 1, ttlSize-1,
						   rt.GetWidth()-mg*2 - 2,
						   1).Translated(rt.min));
		
	}
	
	void Window::Layout() {
		auto cli = GetClientBounds();
		auto sz = GetBounds().GetSize();
		auto mg = cli.min.x;
		moverGrab->SetBounds(AABB2(mg, mg, sz.x - mg*2, cli.min.y - mg));
		moverLeft->SetBounds(AABB2(0, mg, mg, sz.y - mg*2));
		moverRight->SetBounds(AABB2(sz.x - mg, mg, mg, sz.y - mg*2));
		moverTop->SetBounds(AABB2(mg, 0, sz.x - mg*2, mg));
		moverBottom->SetBounds(AABB2(mg, sz.y - mg, sz.x - mg*2, mg));
		moverTopLeft->SetBounds(AABB2(0, 0, mg, mg));
		moverTopRight->SetBounds(AABB2(sz.x - mg, 0, mg, mg));
		moverBottomLeft->SetBounds(AABB2(0, sz.y - mg, mg, mg));
		moverBottomRight->SetBounds(AABB2(sz.x - mg, sz.y - mg, mg, mg));
		
		auto ttlH = cli.min.y - mg;
		closeButton->SetBounds(AABB2(sz.x - mg - ttlH, mg, ttlH, ttlH));
	}
	
} }

