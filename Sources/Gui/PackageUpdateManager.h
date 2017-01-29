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

#include <memory>
#include <string>

namespace spades {
	/**
	 * This class is thread safe.
	 */
	class PackageUpdateManager {
		class UpdateFeed;
		class HttpUpdateFeed;
		class StandardUpdateFeed;

	public:
		static PackageUpdateManager &GetInstance();

		enum class ReadyState {
			/** Automatic update check is disabled. */
			NotLoaded,

			/** The update info is still loading. */
			Loading,

			/** The update info was loaded. */
			Loaded,

			/** An error occured while loading the update info. */
			Error,

			/**
		     * The update info is unavailable because the feed URL is not provided,
		     * or the update feed doesn't provide an information for the current platform.
		     */
			Unavailable
		};

		/** Describes a package version (not a program version, which, however, approximates it). */
		struct VersionInfo {
			int major, minor, revision, build;
			std::string text;

			bool operator<(const VersionInfo &other) const;
			bool operator==(const VersionInfo &other) const {
				return !((*this < other) || (other < *this));
			}
			bool operator>(const VersionInfo &other) const { return other < *this; }

			std::string ToString() const;
		};

		/** Returns the loading state of the update info. */
		ReadyState GetUpdateInfoReadyState();

		/**
		 * Returns a flag indicating whether a newer version of this software is
		 * available for the user's platform.
		 * Always returns `false` if `GetUpdateInfoReadyState() != ReadyState::Loaded`.
		 */
		bool IsUpdateAvailable();

		/**
		 * Returns the version information of the latest version of this software.
		 * Returns an invalid value if `GetUpdateInfoReadyState() != ReadyState::Loaded`.
		 */
		VersionInfo GetLatestVersionInfo();

		/**
		 * Returns the version information of this software package.
		 */
		VersionInfo GetCurrentVersionInfo() { return m_currentVersionInfo; }

		/**
		 * Returns the URL for the web page containing information about the latest version
		 * of the software.
		 * Returns an empty string if `!IsUpdateAvailable()`.
		 */
		std::string GetLatestVersionInfoPageURL();

		/** Initiate an automatic update check. */
		void CheckForUpdate();

	private:
		PackageUpdateManager();
		~PackageUpdateManager();

		ReadyState m_updateInfoReadyState;
		VersionInfo m_latestVersionInfo;
		VersionInfo m_currentVersionInfo;
		std::string m_latestVersionInfoPageURL;

		std::unique_ptr<UpdateFeed> m_updateFeed;
	};
}
