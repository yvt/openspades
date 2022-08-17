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

#include <curl/curl.h>
#include <json/json.h>
#include <memory>
#include <mutex>

#include "PackageUpdateManager.h"

#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/FileManager.h>
#include <Core/Settings.h>
#include <Core/Strings.h>
#include <Core/TMPUtils.h>
#include <Core/Thread.h>
#include <OpenSpades.h>

DEFINE_SPADES_SETTING(cl_checkForUpdates, "0");

namespace spades {
	namespace {
		std::recursive_mutex globalMutex;

		PackageUpdateManager::VersionInfo ParseVersionInfo(const Json::Value &value) {
			if (!value.isObject()) {
				SPRaise("Failed to parse a package version: the value is not an object.");
			}

			const Json::Value &version = value["Version"];
			const Json::Value &versionText = value["VersionText"];
			if (!version.isArray()) {
				SPRaise("Failed to parse a package version: value.Version is not an array.");
			}
			if (!versionText.isString()) {
				SPRaise("Failed to parse a package version: value.VersionText is not a string.");
			}
			if (version.size() != 4) {
				SPRaise(
				  "Failed to parse a package version: value.Version is not a 4-element array.");
			}

			int components[4];
			for (int i = 0; i < 4; ++i) {
				const Json::Value &element = version[i];

				if (!element.isNumeric()) {
					SPRaise("Failed to parse a package version: value.Version[%d] is not a number.",
					        i);
				}

				components[i] = element.asInt();
			}

			return {components[0], components[1], components[2], components[3],
			        versionText.asString()};
		}
	} // namespace

	class PackageUpdateManager::UpdateFeed {
	public:
		virtual void CheckForUpdate() = 0;
		virtual ~UpdateFeed() {}

	protected:
		UpdateFeed(PackageUpdateManager &packageUpdateManager)
		    : m_packageUpdateManager{packageUpdateManager} {}

		PackageUpdateManager &m_packageUpdateManager;

		void ReturnUnavailable() {
			std::lock_guard<std::recursive_mutex> _lock{globalMutex};
			SPAssert(m_packageUpdateManager.m_updateInfoReadyState == ReadyState::Loading);
			m_packageUpdateManager.m_updateInfoReadyState = ReadyState::Unavailable;

			SPLog("Update info is not available for the package and/or the current platform.");
		}
		void ReturnError(const std::string &reason) {
			std::lock_guard<std::recursive_mutex> _lock{globalMutex};
			SPAssert(m_packageUpdateManager.m_updateInfoReadyState == ReadyState::Loading);
			m_packageUpdateManager.m_updateInfoReadyState = ReadyState::Error;

			SPLog("Failed to check for update.: %s", reason.c_str());
		}
		void ReturnVersionInfo(const VersionInfo &info, const std::string &pageURL) {
			std::lock_guard<std::recursive_mutex> _lock{globalMutex};
			SPAssert(m_packageUpdateManager.m_updateInfoReadyState == ReadyState::Loading);
			m_packageUpdateManager.m_updateInfoReadyState = ReadyState::Loaded;
			m_packageUpdateManager.m_latestVersionInfo = info;
			m_packageUpdateManager.m_latestVersionInfoPageURL = pageURL;

			SPLog("The latest available version is %s.", info.ToString().c_str());
		}
	};

	class PackageUpdateManager::HttpUpdateFeed : public UpdateFeed {

		// Protected members of the base class is only accessible by
		// the current object, so...
		void ReturnErrorVeneer(const std::string &reason) { ReturnError(reason); }

		class RequestThread : public Thread {
		public:
			RequestThread(HttpUpdateFeed &parent) : m_parent{parent} {}

			void Run() override {
				std::string responseBuffer;

				try {
					auto curl = std::shared_ptr<CURL>{curl_easy_init(),
					                                  [](CURL *curl) { curl_easy_cleanup(curl); }};

					curl_easy_setopt(
					  curl.get(), CURLOPT_WRITEFUNCTION,
					  static_cast<unsigned long (*)(void *, unsigned long, unsigned long, void *)>(
					    [](void *ptr, unsigned long size, unsigned long nmemb,
					       void *data) -> unsigned long {
						    size_t dataSize = size * nmemb;
						    reinterpret_cast<std::string *>(data)->append(
						      reinterpret_cast<const char *>(ptr), dataSize);
						    return dataSize;
					    }));
					curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBuffer);
					curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, OpenSpades_VER_STR);
					curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0);
					curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_TIME, 30l);
					curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_LIMIT, 15l);
					curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 30l);
					curl_easy_setopt(
					  curl.get(), CURLOPT_XFERINFOFUNCTION,
					  static_cast<int (*)(void *, curl_off_t, curl_off_t, curl_off_t, curl_off_t)>(
					    [](void *, curl_off_t total, curl_off_t downloaded, curl_off_t, curl_off_t) -> int {
					    if (total > 0)
					        SPLog("Downloaded %zd bytes/%zd bytes", downloaded, total);
					    return 0;
					  }));

					m_parent.SetupCURLRequest(curl.get());
					auto reqret = curl_easy_perform(curl.get());

					if (reqret) {
						m_parent.ReturnErrorVeneer(
						  Format("HTTP request error ({0}).", curl_easy_strerror(reqret)));
						return;
					}

					m_parent.ProcessResponse(responseBuffer);
				} catch (std::exception &ex) {
					m_parent.ReturnErrorVeneer(ex.what());
				} catch (...) {
					m_parent.ReturnErrorVeneer("Unknown error.");
				}
			}

		private:
			HttpUpdateFeed &m_parent;
		};

	public:
		void CheckForUpdate() override {
			if (m_thread) {
				m_thread->Join();
			} else {
				m_thread = stmp::make_unique<RequestThread>(*this);
			}
			m_thread->Start();
		}
		~HttpUpdateFeed() override {
			if (m_thread) {
				m_thread->Join();
			}
		}

	protected:
		HttpUpdateFeed(PackageUpdateManager &packageUpdateManager)
		    : UpdateFeed{packageUpdateManager} {}

		virtual void SetupCURLRequest(CURL *handle) = 0;
		virtual void ProcessResponse(const std::string &responseBody) = 0;

	private:
		std::unique_ptr<RequestThread> m_thread;
	};

	class PackageUpdateManager::StandardUpdateFeed : public HttpUpdateFeed {
	public:
		StandardUpdateFeed(PackageUpdateManager &packageUpdateManager, const Json::Value &param)
		    : HttpUpdateFeed{packageUpdateManager} {
			const Json::Value &jsonUrl = param["URL"];
			if (!jsonUrl.isString()) {
				SPRaise("Failed to parse StandardUpdateFeed parameter: URL is not a string.");
			}
			m_url = jsonUrl.asString();
			SPLog("Update feed URL: %s", m_url.c_str());

			const Json::Value &jsonPlatform = param["Platform"];
			if (!jsonUrl.isString()) {
				SPRaise("Failed to parse StandardUpdateFeed parameter: Platform is not a string.");
			}
			m_platform = jsonPlatform.asString();
			SPLog("Package target platform: %s", m_platform.c_str());
		}

	protected:
		void SetupCURLRequest(CURL *handle) override {
			curl_easy_setopt(handle, CURLOPT_URL, m_url.c_str());
		}
		void ProcessResponse(const std::string &responseBody) override {
			Json::Value root;
			if (!Json::Reader{}.parse(responseBody, root, false)) {
				SPRaise("Failed to parse the response.");
			}

			// https://github.com/yvt/openspades/blob/gh-pages/api/version.json
			if (!root.isObject()) {
				SPRaise("Failed to parse the update feed: value is not an object.");
			}

			const Json::Value &jsonVersions = root["Versions"];
			if (!jsonVersions.isArray()) {
				SPRaise("Failed to parse the update feed: value.Versions is not an array.");
			}

			struct Candidate {
				VersionInfo version;
				std::string infoURL;
			};
			stmp::optional<Candidate> best;

			for (Json::UInt i = 0; i < jsonVersions.size(); ++i) {
				const Json::Value &element = jsonVersions[i];
				if (!element.isObject()) {
					SPRaise("Failed to parse the update feed: value.Versions[%d] is not an object.",
					        (int)i);
				}

				VersionInfo versionInfo = ParseVersionInfo(element);

				const Json::Value &jsonPlatforms = element["Platforms"];
				if (!jsonPlatforms.isArray()) {
					SPRaise("Failed to parse the update feed: value.Versions[%d].Platforms is not "
					        "an array.",
					        (int)i);
				}

				bool match = false;
				for (Json::UInt k = 0; k < jsonPlatforms.size(); ++k) {
					const Json::Value &jsonPlatform = jsonPlatforms[k];
					if (!jsonPlatform.isString()) {
						SPRaise("Failed to parse the update feed: value.Versions[%d].Platforms[%d] "
						        "is not a string.",
						        (int)i, (int)k);
					}

					if (jsonPlatform.asString() == m_platform) {
						match = true;
					}
				}

				if (!match || (best && versionInfo < (*best).version)) {
					continue;
				}

				const Json::Value &jsonLinks = element["Links"];
				if (!jsonLinks.isObject()) {
					SPRaise(
					  "Failed to parse the update feed: value.Versions[%d].Links is not an object.",
					  (int)i);
				}

				const Json::Value &jsonLinksInfo = jsonLinks["Info"];
				if (!jsonLinksInfo.isString()) {
					SPRaise("Failed to parse the update feed: value.Versions[%d].Links.Info is not "
					        "a string.",
					        (int)i);
				}

				best.reset(Candidate{versionInfo, jsonLinksInfo.asString()});
			}

			if (best) {
				const Candidate &bestVersion = *best;
				ReturnVersionInfo(bestVersion.version, bestVersion.infoURL);
			} else {
				ReturnUnavailable();
			}
		}

	private:
		std::string m_url;
		std::string m_platform;
	};

	bool PackageUpdateManager::VersionInfo::operator<(const VersionInfo &other) const {
		if (major != other.major) {
			return major < other.major;
		} else if (minor != other.minor) {
			return minor < other.minor;
		} else if (revision != other.revision) {
			return revision < other.revision;
		} else {
			return build < other.build;
		}
	}

	std::string PackageUpdateManager::VersionInfo::ToString() const {
		return Format("{0}.{1}.{2}.{3} ({4})", major, minor, revision, build, text);
	}

	PackageUpdateManager &PackageUpdateManager::GetInstance() {
		static PackageUpdateManager instance;
		return instance;
	}

	PackageUpdateManager::PackageUpdateManager() : m_updateInfoReadyState{ReadyState::NotLoaded} {

		// Load PackageInfo.json
		SPLog("Loading PackageInfo.json");
		std::string data = FileManager::ReadAllBytes("PackageInfo.json");
		Json::Value root;
		if (!Json::Reader{}.parse(data, root, false)) {
			SPRaise("Failed to parse PackageInfo.json.");
		}

		m_currentVersionInfo = ParseVersionInfo(root);
		SPLog("The package version is %s", m_currentVersionInfo.ToString().c_str());

		// Initialize the update feed reader
		const Json::Value &jsonUpdateFeed = root["UpdateFeed"];
		if (jsonUpdateFeed.isObject()) {
			const Json::Value &jsonUpdateFeedType = jsonUpdateFeed["Type"];
			if (!jsonUpdateFeedType.isString()) {
				SPRaise("Failed to parse PackageInfo.json: root.UpdateFeed.Type is not a string.");
			}

			std::string updateFeedType = jsonUpdateFeedType.asString();

			if (updateFeedType == "Standard") {
				m_updateFeed = stmp::make_unique<StandardUpdateFeed>(*this, jsonUpdateFeed);
			} else {
				SPRaise("Failed to parse PackageInfo.json: root.UpdateFeed.Type contains an "
				        "unrecognizable value.");
			}
			SPLog("Update feed type: %s", updateFeedType.c_str());
		} else if (!jsonUpdateFeed.isNull()) {
			SPRaise("Failed to parse PackageInfo.json: root.UpdateFeed is not an object nor null.");
		} else {
			SPLog("Update feed type: (none)");
		}

		if (cl_checkForUpdates) {
			SPLog("Starting an automatic update check.");
			CheckForUpdate();
		} else {
			SPLog("Automatic update check is disabled.");
			if (!m_updateFeed) {
				m_updateInfoReadyState = ReadyState::Unavailable;
			}
		}
	}
	PackageUpdateManager::~PackageUpdateManager() {}

	PackageUpdateManager::ReadyState PackageUpdateManager::GetUpdateInfoReadyState() {
		std::lock_guard<std::recursive_mutex> _lock{globalMutex};
		return m_updateInfoReadyState;
	}

	bool PackageUpdateManager::IsUpdateAvailable() {
		std::lock_guard<std::recursive_mutex> _lock{globalMutex};

		if (m_updateInfoReadyState != ReadyState::Loaded) {
			return false;
		}

		return GetCurrentVersionInfo() < GetLatestVersionInfo();
	}

	PackageUpdateManager::VersionInfo PackageUpdateManager::GetLatestVersionInfo() {
		std::lock_guard<std::recursive_mutex> _lock{globalMutex};
		return m_latestVersionInfo;
	}

	std::string PackageUpdateManager::GetLatestVersionInfoPageURL() {
		std::lock_guard<std::recursive_mutex> _lock{globalMutex};

		if (m_updateInfoReadyState != ReadyState::Loaded) {
			return std::string{};
		}

		return m_latestVersionInfoPageURL;
	}

	void PackageUpdateManager::CheckForUpdate() {
		std::lock_guard<std::recursive_mutex> _lock{globalMutex};
		if (m_updateInfoReadyState == ReadyState::Loading) {
			return;
		}

		if (m_updateFeed) {
			m_updateInfoReadyState = ReadyState::Loading;

			SPLog("Checking for update");
			m_updateFeed->CheckForUpdate();
		} else {
			m_updateInfoReadyState = ReadyState::Unavailable;
			SPLog("Update feed is not available.");
		}
	}
} // namespace spades
