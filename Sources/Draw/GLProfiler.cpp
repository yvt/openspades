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

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <list>
#include <unordered_map>
#include <cstring>

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

			void ResetTimes() { startTimePoint = chrono::high_resolution_clock::now(); }

			double GetWallClockTime() {
				return chrono::duration_cast<chrono::microseconds>(
				         chrono::high_resolution_clock::now() - startTimePoint)
				         .count() /
				       1000000.0;
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

			std::list<Phase>::iterator nextSubphaseIterator;

			double startWallClockTime;
			double endWallClockTime;
			stmp::optional<std::pair<std::size_t, std::size_t>> queryObjectIndices;

			Measurement measurementLatest;
			bool measured = false;

			stmp::optional<Measurement> measurementSaved;

			Phase(const std::string &name) : name{name}, nextSubphaseIterator{subphases.begin()} {}
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

			m_font = m_renderer.RegisterImage("Gfx/Fonts/Debug.png");
			m_white = m_renderer.RegisterImage("Gfx/White.tga");
		}

		GLProfiler::~GLProfiler() {
			SPADES_MARK_FUNCTION();
			for (IGLDevice::UInteger timerQueryObject : m_timerQueryObjects) {
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
						phase.nextSubphaseIterator = phase.subphases.begin();
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
			SPAssert(&GetCurrentPhase() == m_root.get());

			EndPhaseInner();
			SPAssert(m_stack.empty());

			m_active = false;

			FinalizeMeasurement();
		}

		void GLProfiler::FinalizeMeasurement() {
			SPADES_MARK_FUNCTION();
			Phase &root = *m_root;

			// Fill gap
			if (m_settings.r_debugTimingFillGap) {
				struct Traverser {
					GLProfiler &self;
					Traverser(GLProfiler &self) : self{self} {}
					void Traverse(Phase &phase, stmp::optional<std::pair<Phase&, bool>> base) {
						if (!phase.queryObjectIndices) {
							return;
						}
						if (base) {
							auto baseIndices = *(*base).first.queryObjectIndices;
							(*phase.queryObjectIndices).second = (*base).second ? baseIndices.second : baseIndices.first;
						}
						auto it = phase.subphases.begin();
						while (it != phase.subphases.end() && !it->queryObjectIndices) {
							++it;
						}
						while (it != phase.subphases.end()) {
							auto it2 = it; ++it2;
							while (it2 != phase.subphases.end() && !it2->queryObjectIndices) {
								++it2;
							}
							if (it2 == phase.subphases.end()) {
								Traverse(*it, std::pair<Phase&, bool>{phase, true});
							} else {
								Traverse(*it, std::pair<Phase&, bool>{*it2, false});
							}
							it = it2;
						}
					}
				};
				Traverser{*this}.Traverse(root, {});
				// TODO: fill gap for wall clock times
			}

			// Collect GPU time information
			if (m_settings.r_debugTimingGPUTime) {
				if (!m_waitingTimerQueryResult) {
					m_device.EndQuery(IGLDevice::TimeElapsed);
				}

				m_waitingTimerQueryResult = false;

				// are results available?
				for (std::size_t i = 0; i <= m_currentTimerQueryObjectIndex; ++i) {
					if (!m_device.GetQueryObjectUInteger(m_timerQueryObjects[i],
					                                     IGLDevice::QueryResultAvailable)) {
						m_waitingTimerQueryResult = true;
						return;
					}
				}

				double t = 0;
				m_timerQueryTimes.resize(m_currentTimerQueryObjectIndex + 2);
				m_timerQueryTimes[0] = 0.0;
				for (std::size_t i = 0; i <= m_currentTimerQueryObjectIndex; ++i) {
					auto nanoseconds = m_device.GetQueryObjectUInteger64(m_timerQueryObjects.at(i),
					                                                     IGLDevice::QueryResult);
					t += static_cast<double>(nanoseconds) / 1.0e+9;
					m_timerQueryTimes[i + 1] = t;
				}
				struct Traverser {
					GLProfiler &self;
					Traverser(GLProfiler &self) : self{self} {}
					void Traverse(Phase &phase) {
						if (phase.queryObjectIndices) {
							auto indices = *phase.queryObjectIndices;
							double time1 = self.m_timerQueryTimes.at(indices.first);
							double time2 = self.m_timerQueryTimes.at(indices.second);
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
			m_device.BeginQuery(IGLDevice::TimeElapsed,
			                    m_timerQueryObjects[m_currentTimerQueryObjectIndex]);
		}

		void GLProfiler::BeginPhase(const std::string &name, const std::string &description) {
			SPADES_MARK_FUNCTION_DEBUG();

			SPAssert(m_active);

			Phase &current = GetCurrentPhase();

			auto it = current.nextSubphaseIterator;

			for (; it != current.subphases.end(); ++it) {
				if (it->name == name) {
					break;
				}
			}

			if (it == current.subphases.end()) {
				// Create a new subphase
				it = current.subphases.emplace(current.nextSubphaseIterator, name);
			}

			current.nextSubphaseIterator = it;
			++current.nextSubphaseIterator;

			Phase &next = *it;
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

			if (m_settings.r_debugTimingFlush) {
				m_device.Flush();
			}

			phase.startWallClockTime = GetWallClockTime();

			if (m_settings.r_debugTimingGPUTime) {
				NewTimerQuery();
				phase.queryObjectIndices = std::pair<std::size_t, std::size_t>{};
				(*phase.queryObjectIndices).first = m_currentTimerQueryObjectIndex;
				(*phase.queryObjectIndices).second = static_cast<std::size_t>(-1);
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

			if (m_settings.r_debugTimingFlush) {
				m_device.Flush();
			}

			double wallClockTime = GetWallClockTime();

			current.endWallClockTime = wallClockTime;

			current.measurementLatest.totalWallClockTime +=
			  wallClockTime - current.startWallClockTime;
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
						std::snprintf(buf + indent, 511 - indent, "%s - %.3fms / %.3fms",
						              phase.description.c_str(),
						              result.totalGPUTime * 1000. * factor,
						              result.totalWallClockTime * 1000. * factor);
					} else {
						std::snprintf(buf + indent, 511 - indent, "%s - %.3fms",
						              phase.description.c_str(),
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
			if (!root.measurementSaved) {
				// No results yet
				return;
			}

			double factor = 1.0 / (*root.measurementSaved).totalNumFrames;

			struct ResultRenderer {
				GLProfiler &self;
				GLRenderer &renderer;
				bool gpu;
				double factor;
				char buf[512];
				Vector2 cursor{0.0f, 0.0f};
				int column = 0;

				ResultRenderer(GLProfiler &self, double factor)
				    : self{self},
				      renderer{self.m_renderer},
				      gpu{self.m_settings.r_debugTimingGPUTime},
				      factor{factor} {}

				void DrawText(const char *str) {
					client::IImage *font = self.m_font;

					while (*str) {
						char c = *str;
						if (c == '\n') {
							cursor.y += 10.0f;
							if (cursor.y + 10.0f > renderer.ScreenHeight()) {
								cursor.y = 0.0f;
								++column;
							}
							cursor.x = column * 500.0f;
						} else {
							int col = c & 15;
							int row = (c >> 4) - 2;
							renderer.DrawImage(font, cursor,
							                   AABB2{col * 6.0f, row * 10.0f, 6.0f, 10.0f});
							cursor.x += 6.0f;
						}
						++str;
					}
				}

				void Traverse(Phase &phase, int level) {
					if (!phase.measurementSaved) {
						// No results yet
						return;
					}
					Measurement &result = *phase.measurementSaved;
					double time = (gpu ? result.totalGPUTime : result.totalWallClockTime) * factor;

					// draw text
					if (result.totalNumFrames > 0) {
						renderer.SetColorAlphaPremultiplied(Vector4{1.0f, 1.0f, 1.0f, 1.0f});
					} else {
						renderer.SetColorAlphaPremultiplied(Vector4{0.5f, 0.5f, 0.5f, 1.0f});
					}

					int indent = level * 2;
					int timeColumn = 30;
					for (int i = 0; i < indent; i++)
						buf[i] = ' ';
					std::fill(buf, buf + timeColumn, ' ');
					std::strcpy(buf + indent, phase.description.c_str());
					buf[std::strlen(buf)] = ' ';
					std::sprintf(buf + timeColumn, "%7.3fms", time * 1000.);
					DrawText(buf);

					float subphaseTime = 0.0f;
					for (Phase &subphase : phase.subphases) {
						if (!subphase.measurementSaved) {
							continue;
						}
						Measurement &subresult = *subphase.measurementSaved;
						subphaseTime +=
						  (gpu ? subresult.totalGPUTime : subresult.totalWallClockTime) * factor;
					}

					// draw bar
					float barScale = 4000.0 * self.m_settings.r_debugTimingOutputBarScale;
					float boxWidth = static_cast<float>(time * barScale);
					float childBoxWidth = static_cast<float>(subphaseTime * barScale);
					client::IImage *white = self.m_white;

					renderer.SetColorAlphaPremultiplied(Vector4{0.0f, 0.0f, 0.0f, 0.5f});
					renderer.DrawImage(white, AABB2{cursor.x, cursor.y + 1.0f, boxWidth, 8.0f});

					renderer.SetColorAlphaPremultiplied(Vector4{0.0f, 1.0f, 0.0f, 1.0f});
					renderer.DrawImage(white, AABB2{cursor.x, cursor.y + 3.0f, boxWidth, 4.0f});

					renderer.SetColorAlphaPremultiplied(Vector4{1.0f, 0.0f, 0.0f, 1.0f});
					renderer.DrawImage(
					  white, AABB2{cursor.x, cursor.y + 3.0f, boxWidth - childBoxWidth, 4.0f});

					DrawText("\n");

					for (Phase &subphase : phase.subphases) {
						Traverse(subphase, level + 1);
					}
				}

				void Draw(Phase &root) {
					renderer.SetColorAlphaPremultiplied(Vector4{1.0f, 1.0f, 0.0f, 1.0f});
					DrawText("[GLProfiler result] ");
					std::sprintf(buf, "%d sampled frame(s). Showing the per-frame timing.\n",
					             (*root.measurementSaved).totalNumFrames);
					DrawText(buf);
					DrawText("Legends: ");
					DrawText(gpu ? "GPU time" : "wall clock time");
					renderer.SetColorAlphaPremultiplied(Vector4{1.0f, 0.0f, 0.0f, 1.0f});
					DrawText(" Self");
					renderer.SetColorAlphaPremultiplied(Vector4{0.0f, 1.0f, 0.0f, 1.0f});
					DrawText(" Total\n");

					Traverse(root, 0);
				}
			};
			ResultRenderer{*this, factor}.Draw(root);
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
			std::vsnprintf(buf, 2047, format, va);
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
