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
#include <functional>

namespace spades { namespace editor {
	

	class ScrollBar: public UIElement {
		
		class Arrow;
		class Track;
		class Thumb;
		
		Handle<Arrow> arrow1;
		Handle<Arrow> arrow2;
		Handle<Track> track1;
		Handle<Track> track2;
		Handle<Thumb> thumb;
		
		double minValue, maxValue;
		double smallChange, largeChange;
		double value;
		
		std::function<void()> onChange;
		
		void Layout();
		
		float GetTrackLength();
		float GetThumbLength();
		float GetMovableLength();
		double GetValuePerPixel();
		double GetPixelsPerValue();
		
	protected:
		~ScrollBar();
		
		void RenderClient() override;
	public:
		ScrollBar(UIManager *);
		
		void SetRange(double minValue,
					  double maxValue);
		std::pair<double, double>
		GetRange() { return std::make_pair(minValue,
										   maxValue); }
		double GetMinValue() const { return minValue; }
		double GetMaxValue() const { return maxValue; }
		
		void SetValue(double);
		double GetValue() const { return value; }
		
		void SetLargeChange(double v) { largeChange = v; }
		double GetLargeChange() const { return largeChange; }
		void SetSmallChange(double v) { smallChange = v; }
		double GetSmallChange() const { return smallChange; }
		
		void SetChangeHandler(const std::function<void()>&);
		
	};
		
} }
