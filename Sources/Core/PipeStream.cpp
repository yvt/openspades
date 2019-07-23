/*
 Copyright (c) 2019 yvt

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
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>

#include <Core/TMPUtils.h>

#include "PipeStream.h"

namespace spades {
	namespace {
		struct State {
			/** Protects the state from simultaneous access. */
			std::mutex mutex;
			/** Used to notify changes in the state. */
			std::condition_variable condvar;

			/**
			 * The ring buffer.
			 *
			 * Might not be the most efficient choice... But it has a good time complexity.
			 */
			std::deque<char> buffer;

			/** `true` if the writer has hanged up. */
			bool writerHangup = false;

			/** `true` if the reader has hanged up. */
			bool readerHangup = false;
		};

		struct PipeWriter : public IStream {
			std::shared_ptr<State> state;

			PipeWriter(std::shared_ptr<State> state) : state{std::move(state)} {}

			~PipeWriter() {
				{
					std::lock_guard<std::mutex> _lock{state->mutex};
					state->writerHangup = true;
				}

				// The reader must stop waiting
				state->condvar.notify_one();
			}

			void WriteByte(int byte) override {
				auto value = static_cast<std::uint8_t>(byte);
				Write(&value, 1);
			}

			void Write(const void *data, size_t numBytes) override {
				{
					std::lock_guard<std::mutex> _lock{state->mutex};

					auto inputBytes = reinterpret_cast<const char *>(data);

					if (state->readerHangup) {
						return;
					}

					// Allocate the space for incoming bytes
					size_t prevSize = state->buffer.size();
					state->buffer.resize(prevSize + numBytes);

					for (auto it = state->buffer.begin() + prevSize; it != state->buffer.end();
					     ++it) {
						*it = *(inputBytes++);
					}
				}

				// Wake up the reader
				state->condvar.notify_one();
			}
		};

		struct PipeReader : public IStream {
			std::shared_ptr<State> state;

			PipeReader(std::shared_ptr<State> state) : state{std::move(state)} {}

			~PipeReader() {
				std::lock_guard<std::mutex> _lock{state->mutex};
				state->readerHangup = true;

				// Deallocate the ring buffer
				std::deque<char> other;
				state->buffer.swap(other);
			}

			int ReadByte() override {
				std::uint8_t value;
				if (Read(&value, 1)) {
					return value;
				} else {
					return -1;
				}
			}

			size_t Read(void *data, size_t numBytes) override {
				auto outputBytes = reinterpret_cast<char *>(data);
				size_t numActualRead = 0;

				std::unique_lock<std::mutex> lock{state->mutex};

				while (numActualRead < numBytes) {
					state->condvar.wait(
					  lock, [&] { return !state->buffer.empty() || state->writerHangup; });

					if (state->writerHangup && state->buffer.empty()) {
						break;
					}

					// Copy data from the ring buffer
					size_t numAdditionalBytes =
					  std::min(state->buffer.size(), numBytes - numActualRead);
					auto it = state->buffer.begin();
					for (; numAdditionalBytes; --numAdditionalBytes, ++it) {
						*(outputBytes++) = *it;

						++numActualRead;
					}

					// Update the ring buffer
					state->buffer.erase(state->buffer.begin(), it);
				}

				return numActualRead;
			}
		};

	} // namespace

	std::tuple<std::unique_ptr<IStream>, std::unique_ptr<IStream>> CreatePipeStream() {
		auto state = std::make_shared<State>();

		return std::make_tuple(stmp::make_unique<PipeWriter>(state),
		                       stmp::make_unique<PipeReader>(state));
	}
} // namespace spades
