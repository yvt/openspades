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
#include <atomic>
#include <memory>

#include <Core/IStream.h>
#include <Core/TMPUtils.h>

namespace spades {
	class Thread;
	class IRunnable;

	namespace client {

		class GameMap;

		/**
		 * A streaming map loader that can decode a game map in a streaming fashion and
		 * report the progress based on the incomplete decoded data.
		 */
		class GameMapLoader {
		public:
			GameMapLoader();
			~GameMapLoader();

			GameMapLoader(const GameMapLoader &) = delete;
			void operator=(const GameMapLoader &) = delete;

			/**
			 * Adds undecoded data to the internal buffer for decoding.
			 */
			void AddRawChunk(const char *bytes, std::size_t numBytes);

			/**
			 * Notifies the decoder that there is no more undecoded data to process.
			 */
			void MarkEOF();

			/**
			 * Returns `true` if the loading operation is complete, successful or not.
			 */
			bool IsComplete() const;

			/**
			 * Blocks the current thread until the decoding is complete.
			 * Make sure to call `MakeEOF` first or it might cause a deadlock.
			 */
			void WaitComplete();

			/**
			 * Gets how much portion of the map has completed loading.
			 *
			 * @return A value in range `[0, 1]`.
			 */
			float GetProgress();

			/**
			 * Gets the loaded `GameMap` and takes the ownership of it.
			 *
			 * `IsComplete()` must be `true`.
			 *
			 * If an exception occured while decoding the map, the exception will be rethrown when
			 * this method is called.
			 */
			Handle<GameMap> TakeGameMap();

		private:
			struct Decoder;
			struct Result;

			/** A writable stream used to send undecoded data to the decoding thread. */
			std::unique_ptr<IStream> rawDataWriter;

			/** A handle for the decoding thread. */
			std::unique_ptr<Thread> decodingThread;

			/** The `IRunnable` used to create `decodingThread`. Must outlive the thread. */
			std::unique_ptr<IRunnable> decodingThreadRunnable;

			/** The cell for receiving the decode progress. */
			std::atomic<std::uint32_t> progressCell;

			/** The cell for receiving the decode result. */
			stmp::atomic_unique_ptr<Result> resultCell;
		};

	} // namespace client
} // namespace spades
