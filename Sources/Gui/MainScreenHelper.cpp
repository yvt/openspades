/*
 Copyright (c) 2013 yvt

 Portion of the code is based on Serverbrowser.cpp (Copyright (c) 2013 learn_more).

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
#include <cctype>
#include <memory>

#include <curl/curl.h>
#include <json/json.h>

#include "MainScreen.h"
#include "MainScreenHelper.h"
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Settings.h>
#include <Core/Thread.h>
#include <Gui/PackageUpdateManager.h>
#include <OpenSpades.h>

DEFINE_SPADES_SETTING(cl_serverListUrl, "http://services.buildandshoot.com/serverlist.json");

namespace spades {
	namespace {
		struct CURLEasyDeleter {
			void operator()(CURL *ptr) const { curl_easy_cleanup(ptr); }
		};
	} // namespace

	class ServerItem {
		// NetClient::Connect
		std::string mName, mIp, mMap, mGameMode;
		std::string mCountry, mVersion;
		int mPing, mPlayers, mMaxPlayers;

		ServerItem(const std::string &name, const std::string &ip, const std::string &map,
		           const std::string &gameMode, const std::string &country,
		           const std::string &version, int ping, int players, int maxPlayers);

	public:
		static ServerItem *Create(Json::Value &val);

		inline const std::string &GetName() const { return mName; }
		inline const std::string &GetAddress() const { return mIp; }
		inline const std::string &GetMapName() const { return mMap; }
		inline const std::string &GetGameMode() const { return mGameMode; }
		inline const std::string &GetCountryCode() const { return mCountry; }
		inline const std::string &GetVersion() const { return mVersion; }
		inline int GetPing() const { return mPing; }
		inline int GetNumPlayers() const { return mPlayers; }
		inline int GetMaxNumPlayers() const { return mMaxPlayers; }
	};

	ServerItem::ServerItem(const std::string &name, const std::string &ip, const std::string &map,
	                       const std::string &gameMode, const std::string &country,
	                       const std::string &version, int ping, int players, int maxPlayers)
	    : mName(name),
	      mIp(ip),
	      mMap(map),
	      mGameMode(gameMode),
	      mCountry(country),
	      mVersion(version),
	      mPing(ping),
	      mPlayers(players),
	      mMaxPlayers(maxPlayers) {}

	ServerItem *ServerItem::Create(Json::Value &val) {
		ServerItem *item = NULL;
		if (val.type() == Json::objectValue) {
			std::string name, ip, map, gameMode, country, version;
			int ping = 0, players = 0, maxPlayers = 0;

			name = val["name"].asString();
			ip = val["identifier"].asString();
			map = val["map"].asString();
			gameMode = val["game_mode"].asString();
			country = val["country"].asString();
			version = val["game_version"].asString();

			ping = val["latency"].asInt();
			players = val["players_current"].asInt();
			maxPlayers = val["players_max"].asInt();
			item =
			  new ServerItem(name, ip, map, gameMode, country, version, ping, players, maxPlayers);
		}
		return item;
	}

	namespace gui {
		constexpr auto FAVORITE_PATH = "/favorite_servers.json";

		class MainScreenHelper::ServerListQuery final : public Thread {
			Handle<MainScreenHelper> owner;
			std::string buffer;

			void ReturnResult(std::unique_ptr<MainScreenServerList> &&list) {
				owner->resultCell.store(std::move(list));
				owner = NULL; // release owner
			}

			void ProcessResponse() {
				Json::Reader reader;
				Json::Value root;
				auto resp = stmp::make_unique<MainScreenServerList>();

				if (reader.parse(buffer, root, false)) {
					for (Json::Value::iterator it = root.begin(); it != root.end(); ++it) {
						Json::Value &obj = *it;
						std::unique_ptr<ServerItem> srv{ServerItem::Create(obj)};
						if (srv) {
							resp->list.emplace_back(
							  new MainScreenServerItem(
							    srv.get(), owner->favorites.count(srv->GetAddress()) >= 1),
							  false);
						}
					}
				}

				ReturnResult(std::move(resp));
			}

		public:
			ServerListQuery(MainScreenHelper *owner) : owner{owner} {}

			void Run() override {
				try {
					std::unique_ptr<CURL, CURLEasyDeleter> cHandle{curl_easy_init()};
					if (cHandle) {
						size_t (*curlWriteCallback)(void *, size_t, size_t, ServerListQuery *) =
						  [](void *ptr, size_t size, size_t nmemb,
						     ServerListQuery *self) -> size_t {
							size_t numBytes = size * nmemb;
							self->buffer.append(reinterpret_cast<char *>(ptr), numBytes);
							return numBytes;
						};
						curl_easy_setopt(cHandle.get(), CURLOPT_USERAGENT, OpenSpades_VER_STR);
						curl_easy_setopt(cHandle.get(), CURLOPT_URL, cl_serverListUrl.CString());
						curl_easy_setopt(cHandle.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);
						curl_easy_setopt(cHandle.get(), CURLOPT_WRITEDATA, this);
						curl_easy_setopt(cHandle.get(), CURLOPT_LOW_SPEED_TIME, 30l);
						curl_easy_setopt(cHandle.get(), CURLOPT_LOW_SPEED_LIMIT, 15l);
						curl_easy_setopt(cHandle.get(), CURLOPT_CONNECTTIMEOUT, 30l);
						auto reqret = curl_easy_perform(cHandle.get());
						if (CURLE_OK == reqret) {
							ProcessResponse();
						} else {
							SPRaise("HTTP request error (%s).", curl_easy_strerror(reqret));
						}
					} else {
						SPRaise("Failed to create cURL object.");
					}
				} catch (std::exception &ex) {
					auto lst = stmp::make_unique<MainScreenServerList>();
					lst->message = ex.what();
					ReturnResult(std::move(lst));
				} catch (...) {
					auto lst = stmp::make_unique<MainScreenServerList>();
					lst->message = "Unknown error.";
					ReturnResult(std::move(lst));
				}
			}
		};

		MainScreenHelper::MainScreenHelper(MainScreen *scr) : mainScreen(scr), query(NULL) {
			SPADES_MARK_FUNCTION();
			LoadFavorites();
		}

		MainScreenHelper::~MainScreenHelper() {
			SPADES_MARK_FUNCTION();
			if (query) {
				query->MarkForAutoDeletion();
			}
		}

		void MainScreenHelper::MainScreenDestroyed() {
			SPADES_MARK_FUNCTION();
			SaveFavorites();
			mainScreen = NULL;
		}

		void MainScreenHelper::LoadFavorites() {
			SPADES_MARK_FUNCTION();
			Json::Reader reader;

			if (spades::FileManager::FileExists(FAVORITE_PATH)) {
				std::string favs = spades::FileManager::ReadAllBytes(FAVORITE_PATH);
				Json::Value favorite_root;
				if (reader.parse(favs, favorite_root, false)) {
					for (const auto &fav : favorite_root) {
						if (fav.isString())
							favorites.insert(fav.asString());
					}
				}
			}
		}

		void MainScreenHelper::SaveFavorites() {
			SPADES_MARK_FUNCTION();
			Json::StyledWriter writer;
			Json::Value v(Json::ValueType::arrayValue);

			auto fobj = spades::FileManager::OpenForWriting(FAVORITE_PATH);
			for (const auto &favorite : favorites) {
				v.append(Json::Value(favorite));
			}

			fobj->Write(writer.write(v));
		}

		void MainScreenHelper::SetServerFavorite(std::string ip, bool favorite) {
			SPADES_MARK_FUNCTION();
			if (favorite) {
				favorites.insert(ip);
			} else {
				favorites.erase(ip);
			}

			if (result && !result->list.empty()) {
				auto entry = std::find_if(result->list.begin(), result->list.end(),
				                          [&](const Handle<MainScreenServerItem> &entry) {
					                          return entry->GetAddress() == ip;
				                          });
				if (entry != result->list.end()) {
					(*entry)->SetFavorite(favorite);
				}
			}
		}

		bool MainScreenHelper::PollServerListState() {
			SPADES_MARK_FUNCTION();

			// Do we have a new result?
			auto newResult = resultCell.take();
			if (newResult) {
				result = std::move(newResult);
				query->MarkForAutoDeletion();
				query = NULL;
				return true;
			}

			return false;
		}

		void MainScreenHelper::StartQuery() {
			if (query) {
				// There already is an ongoing query
				return;
			}

			query = new ServerListQuery(this);
			query->Start();
		}

#include "Credits.inc" // C++11 raw string literal makes some tools (ex. xgettext, Xcode) misbehave

		std::string MainScreenHelper::GetCredits() {
			std::string html = credits;
			html = Replace(html, "${PACKAGE_STRING}", PACKAGE_STRING);
			return html;
		}

		CScriptArray *MainScreenHelper::GetServerList(std::string sortKey, bool descending) {
			if (result == NULL) {
				return NULL;
			}

			using Item = const Handle<MainScreenServerItem> &;
			std::vector<Handle<MainScreenServerItem>> &lst = result->list;
			if (lst.empty())
				return NULL;

			auto compareFavorite = [&](Item x, Item y) -> stmp::optional<bool> {
				if (x->IsFavorite() && !y->IsFavorite()) {
					return true;
				} else if (!x->IsFavorite() && y->IsFavorite()) {
					return false;
				} else {
					return {};
				}
			};

			auto compareInts = [&](int x, int y) -> bool {
				if (descending) {
					return y < x;
				} else {
					return x < y;
				}
			};

			auto compareStrings = [&](const std::string &x0, const std::string &y0) -> bool {
				const auto &x = descending ? y0 : x0;
				const auto &y = descending ? x0 : y0;
				std::string::size_type t = 0;
				for (t = 0; t < x.length() && t < y.length(); ++t) {
					int xx = std::tolower(x[t]);
					int yy = std::tolower(y[t]);
					if (xx != yy) {
						return xx < yy;
					}
				}
				if (x.length() == y.length()) {
					return false;
				}
				return x.length() < y.length();
			};

			if (!sortKey.empty()) {
				if (sortKey == "Ping") {
					std::stable_sort(lst.begin(), lst.end(), [&](Item x, Item y) {
						return compareFavorite(x, y).value_or(
						  compareInts(x->GetPing(), y->GetPing()));
					});
				} else if (sortKey == "NumPlayers") {
					std::stable_sort(lst.begin(), lst.end(), [&](Item x, Item y) {
						return compareFavorite(x, y).value_or(
						  compareInts(x->GetNumPlayers(), y->GetNumPlayers()));
					});
				} else if (sortKey == "Name") {
					std::stable_sort(lst.begin(), lst.end(), [&](Item x, Item y) {
						return compareFavorite(x, y).value_or(
						  compareStrings(x->GetName(), y->GetName()));
					});
				} else if (sortKey == "MapName") {
					std::stable_sort(lst.begin(), lst.end(), [&](Item x, Item y) {
						return compareFavorite(x, y).value_or(
						  compareStrings(x->GetMapName(), y->GetMapName()));
					});
				} else if (sortKey == "GameMode") {
					std::stable_sort(lst.begin(), lst.end(), [&](Item x, Item y) {
						return compareFavorite(x, y).value_or(
						  compareStrings(x->GetGameMode(), y->GetGameMode()));
					});
				} else if (sortKey == "Protocol") {
					std::stable_sort(lst.begin(), lst.end(), [&](Item x, Item y) {
						return compareFavorite(x, y).value_or(
						  compareStrings(x->GetProtocol(), y->GetProtocol()));
					});
				} else if (sortKey == "Country") {
					std::stable_sort(lst.begin(), lst.end(), [&](Item x, Item y) {
						return compareFavorite(x, y).value_or(
						  compareStrings(x->GetCountry(), y->GetCountry()));
					});
				} else {
					SPRaise("Invalid sort key: %s", sortKey.c_str());
				}
			}

			asIScriptEngine *eng = ScriptManager::GetInstance()->GetEngine();
			asITypeInfo *t = eng->GetTypeInfoByDecl("array<spades::MainScreenServerItem@>");
			SPAssert(t != NULL);
			CScriptArray *arr = CScriptArray::Create(t, static_cast<asUINT>(lst.size()));
			for (size_t i = 0; i < lst.size(); i++) {
				arr->SetValue((asUINT)i, &(lst[i]));
			}
			return arr;
		}

		std::string MainScreenHelper::ConnectServer(std::string hostname, int protocolVersion) {
			if (mainScreen == NULL) {
				return "mainScreen == NULL";
			}
			return mainScreen->Connect(ServerAddress(
			  hostname, protocolVersion == 3 ? ProtocolVersion::v075 : ProtocolVersion::v076));
		}

		std::string MainScreenHelper::GetServerListQueryMessage() {
			if (result == NULL)
				return "";
			return result->message;
		}

		std::string MainScreenHelper::GetPendingErrorMessage() {
			std::string s = errorMessage;
			errorMessage.clear();
			return s;
		}

		PackageUpdateManager &MainScreenHelper::GetPackageUpdateManager() {
			return PackageUpdateManager::GetInstance();
		}

		MainScreenServerList::~MainScreenServerList() {}

		MainScreenServerItem::MainScreenServerItem(ServerItem *item, bool favorite) {
			SPADES_MARK_FUNCTION();
			name = item->GetName();
			address = item->GetAddress();
			mapName = item->GetMapName();
			gameMode = item->GetGameMode();
			country = item->GetCountryCode();
			protocol = item->GetVersion();
			ping = item->GetPing();
			numPlayers = item->GetNumPlayers();
			maxPlayers = item->GetMaxNumPlayers();
			this->favorite = favorite;
		}

		MainScreenServerItem::~MainScreenServerItem() { SPADES_MARK_FUNCTION(); }
	} // namespace gui
} // namespace spades
