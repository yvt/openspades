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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "IGLDevice.h"
#include <Core/Stopwatch.h>

namespace spades {
	namespace client {
		class IImage;
	}
	namespace draw {
		class GLRenderer;
		class GLSettings;
		class GLImage;

		class GLProfiler {
			struct Phase;
			struct Measurement;

			GLSettings &m_settings;
			GLRenderer &m_renderer;
			IGLDevice &m_device;
			bool m_active;
			double m_lastSaveTime;
			bool m_shouldSaveThisFrame;
			bool m_waitingTimerQueryResult;

			client::IImage *m_font;
			client::IImage *m_white;

			Stopwatch m_stopwatch;

			std::unique_ptr<Phase> m_root;
			std::vector<std::reference_wrapper<Phase>> m_stack;

			std::vector<IGLDevice::UInteger> m_timerQueryObjects;
			std::vector<double> m_timerQueryTimes;
			std::size_t m_currentTimerQueryObjectIndex;

			Phase &GetCurrentPhase() { return m_stack.back(); }

			void BeginPhase(const std::string &name, const std::string &description);
			void EndPhase();

			void BeginPhaseInner(Phase &);

			void EndPhaseInner();

			void NewTimerQuery();

			void LogResult(Phase &root);
			void DrawResult(Phase &root);

			void FinalizeMeasurement();

		public:
			GLProfiler(GLRenderer &);
			~GLProfiler();

			void BeginFrame();
			void EndFrame();

			void DrawResult();

			class Context {
				GLProfiler &m_profiler;
				bool m_active;

			public:
				Context(GLProfiler &profiler, const char *format, ...);
				std::string GetProfileMessage();
				~Context();
			};
		};
	}
}
