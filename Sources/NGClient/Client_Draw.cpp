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

#include "Client.h"

#include "Arena.h"
#include <Game/World.h>

#include "NetworkClient.h"
#include <Client/IFont.h>
#include <Client/IRenderer.h>

namespace spades { namespace ngclient {
	
	void Client::RenderLoadingScreen(float dt) {
		SPADES_MARK_FUNCTION();
	
		auto msg = net->GetProgressMessage();
		auto prg = net->GetProgress();
		auto w = renderer->ScreenWidth();
		auto h = renderer->ScreenHeight();
		
		float prgWidth = 499.f;
		float prgHeight = 4.f;
		float prgX = (w - prgWidth) * 0.5f;
		float prgY = h - 40.f;
		
		renderer->SetColorAlphaPremultiplied
		(Vector4(0, 0, 0, 1));
		renderer->DrawImage(nullptr,
							AABB2(0.f, 0.f, w, h));
		
		auto sz = font->Measure(msg);
		font->Draw(msg, Vector2((w - sz.x) * 0.5f, prgY - 10.f - sz.y),
				   1.f, Vector4(1.f, 1.f, 1.f, .8f));
		
		if (prg) {
			renderer->SetColorAlphaPremultiplied
			(Vector4(1.f, 1.f, 1.f, 1.f) * .05f);
			renderer->DrawImage(nullptr,
								AABB2(prgX, prgY, prgWidth, prgHeight));
			
			renderer->SetColorAlphaPremultiplied
			(Vector4(1.f, 1.f, 1.f, 1.f) * .3f);
			renderer->DrawImage(nullptr,
								AABB2(prgX, prgY, prgWidth * *prg, prgHeight));
			
		} else {
			double tt = time * 0.7;
			tt -= floor(tt);
			float centX = static_cast<float>(tt) * (prgWidth + 400.f) - 200.f;;
			
			for(float x = 0; x < prgWidth; x += 5.f) {
				float op = 1.f - fabsf(x - centX) / 200.f;
				op = std::max(op, 0.f) * 0.3f + 0.05f;
				renderer->SetColorAlphaPremultiplied
				(Vector4(1.f, 1.f, 1.f, 1.f) * op);
				renderer->DrawImage(nullptr,
									AABB2(prgX + x, prgY, 5.f, prgHeight));
			}
		}
		
		
	}
	
} }

