/*
 Copyright (c) 2017 yvt

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

#include <Core/Exception.h>

namespace spades {
	namespace client {
		enum class ClientCameraMode {
			/**
			 * The world is not loaded.
			 */
			None,

			/**
			 * That specific camera position (you know what I mean).
			 */
			NotJoined,

			/**
			 * First-person view of the local player.
			 */
			FirstPersonLocal,

			/**
			 * First-person view of a non-local player.
			 */
			FirstPersonFollow,

			/**
			 * Third-person view of the local player (only allowed in a limited situation where
			 * it is enabled by `cg_thirdperson`, or the player is dead).
			 */
			ThirdPersonLocal,

			/**
			 * Third-person view of a non-lcoal player.
			 */
			ThirdPersonFollow,

			/**
			 * Free floating camera view, which is usable only when the user has joined a server
			 * as a spectator.
			 */
			Free,
		};

		inline bool IsThirdPerson(ClientCameraMode mode) {
			switch (mode) {
				case ClientCameraMode::None:
				case ClientCameraMode::NotJoined:
				case ClientCameraMode::FirstPersonLocal:
				case ClientCameraMode::FirstPersonFollow:
				case ClientCameraMode::Free:
					return false;
				case ClientCameraMode::ThirdPersonLocal:
				case ClientCameraMode::ThirdPersonFollow:
					return true;
			}
			SPUnreachable();
		}

		inline bool IsFirstPerson(ClientCameraMode mode) {
			switch (mode) {
				case ClientCameraMode::None:
				case ClientCameraMode::NotJoined:
				case ClientCameraMode::ThirdPersonLocal:
				case ClientCameraMode::ThirdPersonFollow:
				case ClientCameraMode::Free:
					return false;
				case ClientCameraMode::FirstPersonLocal:
				case ClientCameraMode::FirstPersonFollow:
					return true;
			}
			SPUnreachable();
		}

		inline bool HasTargetPlayer(ClientCameraMode mode) {
			switch (mode) {
				case ClientCameraMode::None:
				case ClientCameraMode::NotJoined:
				case ClientCameraMode::Free:
					return false;
				case ClientCameraMode::ThirdPersonLocal:
				case ClientCameraMode::ThirdPersonFollow:
				case ClientCameraMode::FirstPersonLocal:
				case ClientCameraMode::FirstPersonFollow:
					return true;
			}
			SPUnreachable();
		}

		inline bool FollowsNonLocalPlayer(ClientCameraMode mode) {
			switch (mode) {
				case ClientCameraMode::None:
				case ClientCameraMode::NotJoined:
				case ClientCameraMode::Free:
				case ClientCameraMode::ThirdPersonLocal:
				case ClientCameraMode::FirstPersonLocal:
					return false;
				case ClientCameraMode::ThirdPersonFollow:
				case ClientCameraMode::FirstPersonFollow:
					return true;
			}
			SPUnreachable();
		}
	}
}
