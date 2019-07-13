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

namespace spades {

    /** AudioDevice is an interface to the audio device. */
    class AudioDevice {

        /**
         * Loads an audio data from the specified path or load one from
         * the cache, if exists.
         * @param path file-system path.
         */
        AudioChunk @RegisterSound(const string @path) {}

        /** Sets a game world map. */
        GameMap @GameMap {
            set {}
        }

        /** Plays a sound. */
        void Play(AudioChunk @, const Vector3 @origin, const AudioParam @params);

        /**
         * Plays a sound, with the source position specified in the view
         * coordinate space.
         */
        void PlayLocal(AudioChunk @, const Vector3 @origin, const AudioParam @params);

        /** Plays a non-spatialized sound. */
        void PlayLocal(AudioChunk @, const AudioParam @params);

        /** Updates the position of the listener. */
        void Respatialize(const Vector3 @eye, const Vector3 @frontVector, const Vector3 @upVector);
    }

    class AudioParam {
        /** Linear gain of the sound. */
        float volume;

        /**
         * Playback speed of the sound. Doubling this value makes the sound
         * played twice faster.
         */
        float pitch;

        float referenceDistance;
    }

}
