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
#include <exception>

#include "GameMap.h"
#include "GameMapLoader.h"
#include <Core/Debug.h>
#include <Core/DeflateStream.h>
#include <Core/Exception.h>
#include <Core/IRunnable.h>
#include <Core/PipeStream.h>
#include <Core/Thread.h>

namespace spades {
	namespace client {

		struct GameMapLoader::Result {
			// The following fields are mutually exclusive.
			std::exception_ptr exceptionThrown;
			Handle<GameMap> gameMap;
		};

		struct GameMapLoader::Decoder : public IRunnable {
			GameMapLoader &parent;
			StreamHandle rawDataReader;

			Decoder(GameMapLoader &parent, StreamHandle rawDataReader)
			    : parent{parent}, rawDataReader{rawDataReader} {}

			void Run() override {
				SPADES_MARK_FUNCTION();

				std::unique_ptr<Result> result{new Result()};

				try {
					DeflateStream inflate(&*rawDataReader, CompressModeDecompress, false);

					GameMap *gameMapPtr =
					  GameMap::Load(&inflate, [this](int x) { HandleProgress(x); });

					result->gameMap = Handle<GameMap>{gameMapPtr, false};
				} catch (...) {
					// Capture the current exception
					result->exceptionThrown = std::current_exception();
				}

				// Send back the result
				parent.resultCell.store(std::move(result));
			}

			void HandleProgress(int numColumnsLoaded) {
				parent.progressCell.store(numColumnsLoaded);
			}
		};

		GameMapLoader::GameMapLoader() {
			SPADES_MARK_FUNCTION();

			auto pipe = CreatePipeStream();

			rawDataWriter = StreamHandle{std::get<0>(pipe)};
			auto rawDataReader = StreamHandle{std::get<1>(pipe)};

			decodingThreadRunnable.reset(new Decoder(*this, rawDataReader));

			// Drop `rawDataReader` before the thread starts. `StreamHandle`'s internally
			// ref-counted and it's not thread-safe. So, if we don't drop it here, there'll be
			// a data race between the constructor of `Decoder` and the deconstruction of the local
			// variable `rawDataReader`.
			rawDataReader = StreamHandle{};

			decodingThread.reset(new Thread(&*decodingThreadRunnable));
			decodingThread->Start();
		}

		GameMapLoader::~GameMapLoader() {
			SPADES_MARK_FUNCTION();

			// Hang up the writer. This causes the decoder thread to exit gracefully.
			rawDataWriter = StreamHandle{};

			decodingThread->Join();
			decodingThread.reset();
		}

		void GameMapLoader::AddRawChunk(const char *bytes, std::size_t numBytes) {
			SPADES_MARK_FUNCTION();

			if (!rawDataWriter) {
				SPRaise("The raw data channel is already closed.");
			}

			rawDataWriter->Write(bytes, numBytes);
		}

		void GameMapLoader::MarkEOF() {
			SPADES_MARK_FUNCTION();

			if (!rawDataWriter) {
				SPRaise("The raw data channel is already closed.");
			}
			rawDataWriter = StreamHandle{};
		}

		bool GameMapLoader::IsComplete() { return resultCell.operator bool(); }

		void GameMapLoader::WaitComplete() {
			SPADES_MARK_FUNCTION();

			decodingThread->Join();

			SPAssert(IsComplete());
		}

		float GameMapLoader::GetProgress() {
			return static_cast<float>(progressCell.load(std::memory_order_relaxed)) / (512 * 512);
		}

		GameMap *GameMapLoader::TakeGameMap() {
			SPADES_MARK_FUNCTION();

			SPAssert(IsComplete());

			std::unique_ptr<Result> result = resultCell.take();
			SPAssert(result);

			if (result->gameMap) {
				return result->gameMap.Unmanage();
			} else {
				std::rethrow_exception(result->exceptionThrown);
			}
		}

	} // namespace client
} // namespace spades
