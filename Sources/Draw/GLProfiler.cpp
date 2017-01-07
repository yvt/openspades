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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <list>
#include <unordered_map>
#include <ctime>
#include <chrono>

#include "GLProfiler.h"

#include "GLRenderer.h"
#include "GLSettings.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/TMPUtils.h>

namespace chrono = std::chrono;

namespace spades {
	namespace draw {

		namespace {
			chrono::high_resolution_clock::time_point startTimePoint;

			void ResetTimes() {
				startTimePoint = chrono::high_resolution_clock::now();
			}

			double GetWallClockTime() {
				return chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - startTimePoint).count() / 1000000.0;
			}
		}

		struct GLProfiler::Measurement {
			double totalWallClockTime = 0.0;
			double totalGPUTime = 0.0;
			int totalNumFrames = 0;
		};

		struct GLProfiler::Phase {
			const std::string name;
			std::string description;

			// can't use vector here; a reference to a vector's element can be invalidated
			std::list<Phase> subphases;

			/** Looks up a subphase by its name. */
			std::unordered_map<std::string, std::reference_wrapper<Phase>> subphaseMap;

			double startWallClockTime;
			stmp::optional<std::pair<std::size_t, std::size_t>> queryObjectIndices;

			Measurement measurementLatest;
			bool measured = false;

			stmp::optional<Measurement> measurementSaved;

			Phase(const std::string &name) : name{name} {}
		};

		GLProfiler::GLProfiler(GLRenderer &renderer)
		    : m_settings{renderer.GetSettings()},
		      m_renderer{renderer},
		      m_device{*renderer.GetGLDevice()},
		      m_active{false},
		      m_lastSaveTime{0.0},
		      m_shouldSaveThisFrame{false},
			  m_waitingTimerQueryResult{false} {
			SPADES_MARK_FUNCTION();
		}

		GLProfiler::~GLProfiler() {
			SPADES_MARK_FUNCTION();
			for (IGLDevice::UInteger timerQueryObject: m_timerQueryObjects) {
				m_device.DeleteQuery(timerQueryObject);
			}
		}

		void GLProfiler::BeginFrame() {
			SPADES_MARK_FUNCTION();

			if (!m_settings.r_debugTiming) {
				// Clear history
				m_root.reset();
				m_waitingTimerQueryResult = false;
				return;
			}

			if (m_waitingTimerQueryResult) {
				FinalizeMeasurement();

				// Still waiting?
				if (m_waitingTimerQueryResult) {
					return;
				}
			}

			SPAssert(m_stack.empty());
			SPAssert(!m_active);

			m_active = true;

			if (m_settings.r_debugTimingAverage) {
				m_shouldSaveThisFrame = m_stopwatch.GetTime() > m_lastSaveTime + 1.0;
			} else {
				// Clear history each frame in this case
				m_root.reset();

				m_shouldSaveThisFrame = true;
			}

			ResetTimes();

			if (m_settings.r_debugTimingGPUTime) {
				m_currentTimerQueryObjectIndex = 0;
				if (m_timerQueryObjects.empty()) {
					m_timerQueryObjects.push_back(m_device.GenQuery());
				}
				m_device.BeginQuery(IGLDevice::TimeElapsed, m_timerQueryObjects[0]);
			}

			if (m_root) {
				struct Traverser {
					GLProfiler &self;
					Traverser(GLProfiler &self) : self{self} {}
					void Traverse(Phase &phase) {
						phase.measured = false;
						phase.queryObjectIndices.reset();
						for (Phase &subphase : phase.subphases) {
							Traverse(subphase);
						}
					}
				};
				Traverser{*this}.Traverse(*m_root);
			}

			if (!m_root) {
				m_root.reset(new Phase{"Frame"});
				m_root->description = "Frame";
			}

			BeginPhaseInner(*m_root);

			m_stack.emplace_back(*m_root);
		}

		void GLProfiler::EndFrame() {
			SPADES_MARK_FUNCTION();

			if (!m_active) {
				return;
			}

			SPAssert(m_stack.size() == 1);

			Phase &root = GetCurrentPhase();
			SPAssert(&root == m_root.get());

			EndPhaseInner();
			SPAssert(m_stack.empty());

			m_active = false;

			FinalizeMeasurement();
		}

		void GLProfiler::FinalizeMeasurement() {
			SPADES_MARK_FUNCTION();
			Phase &root = *m_root;

			// Collect GPU time information
			if (m_settings.r_debugTimingGPUTime) {
				if (!m_waitingTimerQueryResult) {
					m_device.EndQuery(IGLDevice::TimeElapsed);
				}

				m_waitingTimerQueryResult = false;

				// are results available?
				for (std::size_t i = 0; i <= m_currentTimerQueryObjectIndex; ++i) {
					if (!m_device.GetQueryObjectUInteger(m_timerQueryObjects[i], IGLDevice::QueryResultAvailable)) {
						m_waitingTimerQueryResult = true;
						return;
					}
				}

				double t = 0;
				m_timerQueryTimes.resize(m_currentTimerQueryObjectIndex + 2);
				m_timerQueryTimes[0] = 0.0;
				for (std::size_t i = 0; i <= m_currentTimerQueryObjectIndex; ++i) {
					auto nanoseconds = m_device.GetQueryObjectUInteger64(m_timerQueryObjects[i], IGLDevice::QueryResult);
					t += static_cast<double>(nanoseconds) / 1.0e+9;
					m_timerQueryTimes[i + 1] = t;
				}
				struct Traverser {
					GLProfiler &self;
					Traverser(GLProfiler &self) : self{self} {}
					void Traverse(Phase &phase) {
						if (phase.queryObjectIndices) {
							auto indices = *phase.queryObjectIndices;
							double time1 = self.m_timerQueryTimes[indices.first];
							double time2 = self.m_timerQueryTimes[indices.second];
							phase.measurementLatest.totalGPUTime += time2 - time1;
						}

						for (Phase &subphase : phase.subphases) {
							Traverse(subphase);
						}
					}
				};
				Traverser{*this}.Traverse(root);
			} else {
				m_waitingTimerQueryResult = false;
			}

			if (m_shouldSaveThisFrame) {
				struct Traverser {
					GLProfiler &self;
					Traverser(GLProfiler &self) : self{self} {}
					void Traverse(Phase &phase) {
						phase.measurementSaved = phase.measurementLatest;
						phase.measurementLatest = Measurement{};
						for (Phase &subphase : phase.subphases) {
							Traverse(subphase);
						}
					}
				};
				Traverser{*this}.Traverse(root);

				m_lastSaveTime = m_stopwatch.GetTime();

				// Output the result to the system log
				if (m_settings.r_debugTimingOutputLog) {
					LogResult(root);
				}
			}
		}

		void GLProfiler::NewTimerQuery() {
			SPADES_MARK_FUNCTION_DEBUG();

			m_device.EndQuery(IGLDevice::TimeElapsed);
			++m_currentTimerQueryObjectIndex;

			if (m_currentTimerQueryObjectIndex >= m_timerQueryObjects.size()) {
				m_timerQueryObjects.push_back(m_device.GenQuery());
			}
			m_device.BeginQuery(IGLDevice::TimeElapsed, m_timerQueryObjects[m_currentTimerQueryObjectIndex]);
		}

		void GLProfiler::BeginPhase(const std::string &name, const std::string &description) {
			SPADES_MARK_FUNCTION_DEBUG();

			SPAssert(m_active);

			Phase &current = GetCurrentPhase();

			// Look up existing Phase object (only when r_debugTimingAverage != 0)
			auto it = current.subphaseMap.find(name);

			if (it == current.subphaseMap.end()) {
				// Create a new subphase
				auto subphasesIt = current.subphases.emplace(current.subphases.end(), name);
				auto insertResult = current.subphaseMap.emplace(name, std::ref(*subphasesIt));
				SPAssert(insertResult.second);
				it = insertResult.first;
			}

			Phase &next = it->second;
			m_stack.emplace_back(next);

			if (next.measured) {
				SPRaise("Cannot measure the timing of phase '%s' twice", name.c_str());
			}

			next.measured = true;
			next.description = description;

			BeginPhaseInner(next);
		}

		void GLProfiler::BeginPhaseInner(Phase &phase) {
			SPADES_MARK_FUNCTION_DEBUG();

			phase.startWallClockTime = GetWallClockTime();

			if (m_settings.r_debugTimingGPUTime) {
				NewTimerQuery();
				phase.queryObjectIndices = std::pair<std::size_t, std::size_t>{};
				(*phase.queryObjectIndices).first = m_currentTimerQueryObjectIndex;
			}
		}

		void GLProfiler::EndPhase() {
			SPADES_MARK_FUNCTION_DEBUG();

			SPAssert(m_active);
			SPAssert(m_stack.size() > 1);

			EndPhaseInner();
		}

		void GLProfiler::EndPhaseInner() {
			SPADES_MARK_FUNCTION_DEBUG();

			SPAssert(!m_stack.empty());

			Phase &current = GetCurrentPhase();

			double wallClockTime = GetWallClockTime();

			current.measurementLatest.totalWallClockTime += wallClockTime - current.startWallClockTime;
			current.measurementLatest.totalNumFrames += 1;

			if (m_settings.r_debugTimingGPUTime) {
				SPAssert(current.queryObjectIndices);
				NewTimerQuery();
				(*current.queryObjectIndices).second = m_currentTimerQueryObjectIndex;
			}

			m_stack.pop_back();
		}

		void GLProfiler::LogResult(Phase &root) {
			if (!root.measurementSaved) {
				// No results yet
				return;
			}

			double factor = 1.0 / (*root.measurementSaved).totalNumFrames;
			SPLog("---- Start of GLProfiler Result ----");
			SPLog("(%d sampled frame(s). Showing the per-frame timing)",
			      (*root.measurementSaved).totalNumFrames);

			if (m_settings.r_debugTimingGPUTime) {
				SPLog("(GPU time / wall clock time)");
			} else {
				SPLog("(wall clock time)");
			}

			struct Traverser {
				GLProfiler &self;
				double factor;
				char buf[512];

				Traverser(GLProfiler &self, double factor) : self{self}, factor{factor} {}
				void Traverse(Phase &phase, int level) {
					if (!phase.measurementSaved) {
						// No results yet
						return;
					}
					Measurement &result = *phase.measurementSaved;

					int indent = level * 2;
					for (int i = 0; i < indent; i++)
						buf[i] = ' ';
					buf[511] = 0;
					if (self.m_settings.r_debugTimingGPUTime) {
						snprintf(buf + indent, 511 - indent, "%s - %.3fms / %.3fms", phase.description.c_str(),
								 result.totalGPUTime * 1000. * factor,
								 result.totalWallClockTime * 1000. * factor);
					} else {
						snprintf(buf + indent, 511 - indent, "%s - %.3fms", phase.description.c_str(),
								 result.totalWallClockTime * 1000. * factor);
					}
					SPLog("%s", buf);

					for (Phase &subphase : phase.subphases) {
						Traverse(subphase, level + 1);
					}
				}
			};
			Traverser{*this, factor}.Traverse(root, 0);

			SPLog("---- End of GLProfiler Result ----");
		}

		void GLProfiler::DrawResult() {
			if (m_root) {
				DrawResult(*m_root);
			}
		}

		void GLProfiler::DrawResult(Phase &root) {
			// TODO: draw results
		}

		GLProfiler::Context::Context(GLProfiler &profiler, const char *format, ...)
		    : m_profiler{profiler}, m_active{false} {
			SPADES_MARK_FUNCTION_DEBUG();

			if (!profiler.m_active) {
				return;
			}

			m_active = true;

			char buf[2048];
			va_list va;
			va_start(va, format);
			buf[2047] = 0;
			vsnprintf(buf, 2047, format, va);
			va_end(va);

			profiler.BeginPhase(format, buf);
		}
		GLProfiler::Context::~Context() {
			SPADES_MARK_FUNCTION_DEBUG();

			if (!m_active) {
				return;
			}

			m_profiler.EndPhase();
		}
	}
}
