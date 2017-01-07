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
			std::string name;
			std::string description;

			// can't use vector here; a reference to a vector's element can be invalidated
			std::list<Phase> subphases;

			/** Looks up a subphase by its name. */
			std::unordered_map<std::string, std::reference_wrapper<Phase>> subphaseMap;

			double startWallClockTime;
			double startCPUTime;

			Measurement measurementLatest;

			stmp::optional<Measurement> measurementSaved;
		};

		GLProfiler::GLProfiler(GLRenderer &renderer)
		    : m_settings{renderer.GetSettings()},
		      m_renderer{renderer},
		      m_device{*renderer.GetGLDevice()},
		      m_active{false},
		      m_lastSaveTime{0.0},
		      m_shouldSaveThisFrame{false} {
			SPADES_MARK_FUNCTION();

			m_root.reset(new Phase());
			m_root->name = "Frame";
		}

		GLProfiler::~GLProfiler() {}

		void GLProfiler::BeginFrame() {
			SPADES_MARK_FUNCTION();

			if (!m_settings.r_debugTiming) {
				// Clear history
				m_root.reset();
				return;
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

			if (!m_root) {
				m_root.reset(new Phase());
				m_root->name = "Frame";
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

			if (m_settings.r_debugTimingOutputLog) {
				LogResult(root);
			}

			if (m_shouldSaveThisFrame) {
				m_lastSaveTime = m_stopwatch.GetTime();
			}

			m_active = false;
		}

		void GLProfiler::BeginPhase(const std::string &name, const std::string &description) {
			SPADES_MARK_FUNCTION_DEBUG();

			SPAssert(m_active);

			Phase &current = GetCurrentPhase();

			// Look up existing Phase object (only when r_debugTimingAverage != 0)
			auto it = current.subphaseMap.find(name);

			if (it == current.subphaseMap.end()) {
				// Create a new subphase
				current.subphases.emplace_back();

				auto subphasesIt = current.subphases.emplace(current.subphases.end());
				auto insertResult = current.subphaseMap.emplace(name, std::ref(*subphasesIt));
				SPAssert(insertResult.second);
				it = insertResult.first;

				it->second.get().name = name;
			}

			Phase &next = it->second;
			m_stack.emplace_back(next);

			next.description = description;

			BeginPhaseInner(next);
		}

		void GLProfiler::BeginPhaseInner(Phase &phase) {
			phase.startWallClockTime = GetWallClockTime();
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

			if (m_shouldSaveThisFrame) {
				current.measurementSaved = current.measurementLatest;
				current.measurementLatest = Measurement{};
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
			SPLog("(wall clock time)");

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
					snprintf(buf + indent, 511 - indent, "%s - %.3fms", phase.description.c_str(),
					         result.totalWallClockTime * 1000. * factor);
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
