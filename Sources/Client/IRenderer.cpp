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

#include "IRenderer.h"

#include <Core/Exception.h>

namespace spades {
	namespace client {
		IRenderer::IRenderer() {}

		void IRenderer::Init() {
			SPADES_MARK_FUNCTION();

			InitLowLevel();
		}

		void IRenderer::Shutdown() {
			SPADES_MARK_FUNCTION();

			ShutdownLowLevel();
		}

		IRenderer::State2D &IRenderer::GetCurrentState() {
			SPADES_MARK_FUNCTION_DEBUG();

			if (m_stateStack.empty()) {
				ResetStack();
			}

			return m_stateStack.back();
		}

		void IRenderer::ResetStack() {
			SPADES_MARK_FUNCTION();

			m_stateStack.resize(1);
			GetCurrentState() = {AABB2{0.0f, 0.0f, ScreenWidth(), ScreenHeight()},
			                     Vector2{1.0f, 1.0f}, Vector2{0.0f, 0.0f}};
		}

		void IRenderer::Save() {
			SPADES_MARK_FUNCTION();

			if (m_stateStack.size() > 256) {
				SPRaise("The drawing state stack is full.");
			}

			m_stateStack.push_back(State2D{GetCurrentState()});
		}

		void IRenderer::Restore() {
			SPADES_MARK_FUNCTION();

			if (m_stateStack.size() == 1) {
				SPRaise("The drawing state stack is empty.");
			}
			m_stateStack.pop_back();

			SetScissorLowLevel(GetCurrentState().scissorRect);
		}

		Vector2 IRenderer::TransformPoint(const Vector2 &input) {
			SPADES_MARK_FUNCTION_DEBUG();

			State2D &state = GetCurrentState();
			return input * state.transformScaling + state.transformTranslation;
		}

		Vector2 IRenderer::TransformVector(const Vector2 &input) {
			SPADES_MARK_FUNCTION_DEBUG();

			return input * GetCurrentState().transformScaling;
		}

		AABB2 IRenderer::Transform(const AABB2 &input) {
			SPADES_MARK_FUNCTION_DEBUG();

			return AABB2{TransformPoint(input.min), TransformPoint(input.max)};
		}

		void IRenderer::Scale(const Vector2 &factor) {
			SPADES_MARK_FUNCTION();

			State2D &state = GetCurrentState();
			state.transformScaling *= factor;
		}

		void IRenderer::Translate(const Vector2 &offset) {
			SPADES_MARK_FUNCTION();

			State2D &state = GetCurrentState();
			state.transformTranslation += offset * state.transformScaling;
		}

		void IRenderer::DrawImage(IImage *image, const Vector2 &outTopLeft) {
			SPADES_MARK_FUNCTION();

			if (image == nullptr) {
				SPRaise("Size must be specified when null image is provided");
			}

			DrawImage(image,
			          AABB2(outTopLeft.x, outTopLeft.y, image->GetWidth(), image->GetHeight()),
			          AABB2(0, 0, image->GetWidth(), image->GetHeight()));
		}

		void IRenderer::DrawImage(IImage *image, const AABB2 &outRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image, outRect,
			          AABB2(0, 0, image ? image->GetWidth() : 0, image ? image->GetHeight() : 0));
		}

		void IRenderer::DrawImage(IImage *image, const Vector2 &outTopLeft, const AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image,
			          AABB2(outTopLeft.x, outTopLeft.y, inRect.GetWidth(), inRect.GetHeight()),
			          inRect);
		}

		void IRenderer::DrawImage(IImage *image, const AABB2 &outRect, const AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImageLowLevel(image, Transform(outRect), inRect);
		}

		void IRenderer::DrawImage(IImage *image, const Vector2 &outTopLeft,
		                          const Vector2 &outTopRight, const Vector2 &outBottomLeft,
		                          const AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImageLowLevel(image, TransformPoint(outTopLeft), TransformPoint(outTopRight),
			                  TransformPoint(outBottomLeft), inRect);
		}

		void IRenderer::DrawImageLowLevel(IImage *image, const AABB2 &outRect,
		                                  const AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image, Vector2::Make(outRect.GetMinX(), outRect.GetMinY()),
			          Vector2::Make(outRect.GetMaxX(), outRect.GetMinY()),
			          Vector2::Make(outRect.GetMinX(), outRect.GetMaxY()), inRect);
		}

		void IRenderer::SetScissor(const AABB2 &scissorRect) {
			SPADES_MARK_FUNCTION();

			AABB2 transformed = Transform(scissorRect);
			GetCurrentState().scissorRect = transformed;
			SetScissorLowLevel(transformed);
		}
	}
}
