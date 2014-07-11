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

#include "ScrollBar.h"
#include "Buttons.h"

namespace spades { namespace editor {
	
	enum class ArrowDirection {
		Up, Down
	};
	
	class ScrollBar::Arrow: public ButtonBase {
		ArrowDirection dir;
		Handle<client::IImage> img;
	protected:
		~Arrow() { }
		void RenderClient() override {
			ButtonBase::RenderClient();
			
			auto rt = GetScreenBounds();
			auto r = ToHandle(GetManager().GetRenderer());
			auto pt = rt.GetMid();
			auto sz = img->GetWidth() * .5f;
			AABB2 inRect(0, 0, sz*2.f, sz*2.f);
			
			auto state = GetState();
			switch (state) {
				case ButtonState::Default:
					r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.3f);
					break;
				case ButtonState::Hover:
					r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.4f);
					break;
				case ButtonState::Pressed:
					r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.5f);
					break;
			}
			
			if (dir == ArrowDirection::Up) {
				r->DrawImage(img,
							 pt + Vector2(-sz, -sz),
							 pt + Vector2(+sz, -sz),
							 pt + Vector2(-sz, +sz),
							 inRect);
			} else if (dir == ArrowDirection::Down) {
				r->DrawImage(img,
							 pt + Vector2(+sz, +sz),
							 pt + Vector2(-sz, +sz),
							 pt + Vector2(+sz, -sz),
							 inRect);
			}
		}
	public:
		Arrow(UIManager *e,
			  ArrowDirection d):
		ButtonBase(e), dir(d) {
			SetAutoRepeat(true);
			img.Set(e->GetRenderer()->RegisterImage("Gfx/UI/ScrollArrow.png"),
					false);
		}
	};
	
	
	class ScrollBar::Track: public ButtonBase {
	protected:
		~Track() { }
	public:
		Track(UIManager *e):
		ButtonBase(e) {
			SetAutoRepeat(true);
		}
	};
	
	
	class ScrollBar::Thumb: public ButtonBase {
		ScrollBar& scrollbar;
		bool drag = false;
		bool hover = false;
		Vector2 lastPt { 0, 0 };
		double lastValue = 0;
	protected:
		~Thumb() { }
		void RenderClient() override {
			auto rt = GetScreenBounds();
			auto r = ToHandle(GetManager().GetRenderer());
			if (hover && drag) {
				r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.5f);
			} else if (hover) {
				r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.4f);
			} else {
				r->SetColorAlphaPremultiplied(Vector4(0,0,0,1)*.3f);
			}
			r->DrawImage(nullptr, AABB2(rt.GetMid().x - 4.f, rt.GetMinY(),
										8.f, rt.GetHeight()));
		}
		void OnMouseEnter() override {
			hover = true;
		}
		void OnMouseLeave() override {
			hover = false;
		}
		void OnMouseDown(MouseButton b, const Vector2& p) override {
			if (b == MouseButton::Left) {
				drag = true;
				lastPt = GetManager().GetMousePosition();
				lastValue = scrollbar.value;
			}
		}
		void OnMouseUp(MouseButton b, const Vector2& p) override {
			if (b == MouseButton::Left && drag) {
				drag = false;
			}
		}
		void OnMouseMove(const Vector2& p) override {
			if (drag) {
				auto diff = GetManager().GetMousePosition() - lastPt;
				auto valDiff = diff.y * scrollbar.GetValuePerPixel();
				scrollbar.SetValue(lastValue + valDiff);
			}
		}
	public:
		Thumb(UIManager *e,
			  ScrollBar& sb):
		ButtonBase(e),
		scrollbar(sb) { }
	};
	
	
	
	ScrollBar::ScrollBar(UIManager *e):
	UIElement(e) {
		arrow1 = MakeHandle<Arrow>(e, ArrowDirection::Up);
		arrow2 = MakeHandle<Arrow>(e, ArrowDirection::Down);
		track1 = MakeHandle<Track>(e);
		track2 = MakeHandle<Track>(e);
		thumb = MakeHandle<Thumb>(e, *this);
		AddChildToFront(arrow1);
		AddChildToFront(arrow2);
		AddChildToFront(track1);
		AddChildToFront(track2);
		AddChildToFront(thumb);
		
		minValue = 0.;
		maxValue = 100.;
		smallChange = 10.;
		largeChange = 20.;
		value = 0.0;
		
		arrow1->SetActivateHandler([&]() {
			SetValue(GetValue() - smallChange);
		});
		arrow2->SetActivateHandler([&]() {
			SetValue(GetValue() + smallChange);
		});
		track1->SetActivateHandler([&]() {
			SetValue(GetValue() - largeChange);
		});
		track2->SetActivateHandler([&]() {
			SetValue(GetValue() + largeChange);
		});
	}
	
	ScrollBar::~ScrollBar() {
		
	}
	
	void ScrollBar::RenderClient() {
		Layout();
	}
	
	float ScrollBar::GetTrackLength() {
		return GetBounds().GetHeight() - 32.f;
	}
	
	float ScrollBar::GetThumbLength() {
		auto range = maxValue - minValue + largeChange;
		auto per = largeChange / range;
		auto len = static_cast<float>(per * GetTrackLength());
		len = std::max(len, 16.f);
		return len;
	}
	
	float ScrollBar::GetMovableLength() {
		return GetTrackLength() - GetThumbLength();
	}
	
	double ScrollBar::GetValuePerPixel() {
		auto range = maxValue - minValue;
		double movable = GetMovableLength();
		if (movable == 0.) return 0.0;
		return range / movable;
	}
	
	double ScrollBar::GetPixelsPerValue() {
		auto range = maxValue - minValue;
		double movable = GetMovableLength();
		if (range == 0.) return 0.0;
		return movable / range;
	}
	
	void ScrollBar::SetValue(double v) {
		if (v == value) return;
		
		if (v < minValue) v = minValue;
		if (v > maxValue) v = maxValue;
		value = v;
		if (onChange)
			onChange();
	}
	
	void ScrollBar::SetRange(double minValue, double maxValue) {
		this->minValue = minValue;
		this->maxValue = maxValue;
		
		SetValue(value);
	}
	
	void ScrollBar::SetChangeHandler(const std::function<void ()> &h) {
		onChange = h;
	}
	
	void ScrollBar::Layout() {
		if (value < minValue) value = minValue;
		if (value > maxValue) value = maxValue;
		
		auto sz = GetBounds().GetSize();
		auto arrowSize = (sz.y - GetTrackLength()) * .5f;
		
		arrow1->SetBounds(AABB2(0, 0, sz.x, arrowSize));
		arrow2->SetBounds(AABB2(0, sz.y - arrowSize, sz.x, arrowSize));
		
		float thumbSize = GetThumbLength();
		float thumbPos = GetPixelsPerValue() * (value - minValue);
		
		track1->SetBounds(AABB2(0, arrowSize, sz.x, thumbPos
								));
		track2->SetBounds(AABB2(0, arrowSize + thumbPos + thumbSize,
								sz.x, GetMovableLength() - thumbPos));
		
		thumb->SetBounds(AABB2(0, thumbPos + arrowSize,
							   sz.x, thumbSize));
		
	}
	
} }

