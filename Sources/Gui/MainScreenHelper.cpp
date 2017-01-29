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

#include <curl/curl.h>
#include <json/json.h>

#include "MainScreen.h"
#include "MainScreenHelper.h"
#include <Core/AutoLocker.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Settings.h>
#include <Core/Thread.h>
#include <Gui/PackageUpdateManager.h>
#include <OpenSpades.h>

DEFINE_SPADES_SETTING(cl_serverListUrl, "http://services.buildandshoot.com/serverlist.json");

namespace spades {

	class Serveritem {
		// NetClient::Connect
		std::string mName, mIp, mMap, mGameMode;
		std::string mCountry, mVersion;
		int mPing, mPlayers, mMaxPlayers;

		Serveritem(const std::string &name, const std::string &ip, const std::string &map,
		           const std::string &gameMode, const std::string &country,
		           const std::string &version, int ping, int players, int maxPlayers);

	public:
		static Serveritem *create(Json::Value &val);

		inline const std::string &Name() const { return mName; }
		inline const std::string &Ip() const { return mIp; }
		inline const std::string &Map() const { return mMap; }
		inline const std::string &GameMode() const { return mGameMode; }
		inline const std::string &Country() const { return mCountry; }
		inline const std::string &Version() const { return mVersion; }
		inline int Ping() const { return mPing; }
		inline int Players() const { return mPlayers; }
		inline int MaxPlayers() const { return mMaxPlayers; }
	};

	Serveritem::Serveritem(const std::string &name, const std::string &ip, const std::string &map,
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

	Serveritem *Serveritem::create(Json::Value &val) {
		Serveritem *item = NULL;
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
			  new Serveritem(name, ip, map, gameMode, country, version, ping, players, maxPlayers);
		}
		return item;
	}

	namespace gui {
		constexpr auto FAVORITE_PATH = "/favorite_servers.json";

		class MainScreenHelper::ServerListQuery : public Thread {
			Mutex infoLock;
			Handle<MainScreenHelper> owner;
			std::string buffer;

			static size_t curlWriteCallback(void *ptr, size_t size, size_t nmemb,
			                                ServerListQuery *query) {
				size_t dataSize = size * nmemb;
				return query->writeCallback(ptr, dataSize);
			}

			size_t writeCallback(void *ptr, size_t size) {

				buffer.append(reinterpret_cast<char *>(ptr), size);
				return size;
			}

			void ReturnResult(MainScreenServerList *list) {
				AutoLocker lock(&(owner->newResultArrayLock));
				delete owner->newResult;
				owner->newResult = list;
				owner = NULL; // release owner
			}

			void ProcessResponse() {
				Json::Reader reader;
				Json::Value root;
				MainScreenServerList *resp = new MainScreenServerList();
				try {
					if (reader.parse(buffer, root, false)) {
						for (Json::Value::iterator it = root.begin(); it != root.end(); ++it) {
							Json::Value &obj = *it;
							Serveritem *srv = Serveritem::create(obj);
							if (srv) {
								resp->list.push_back(new MainScreenServerItem(
								  srv, owner->favorites.count(srv->Ip()) >= 1));
								delete srv;
							}
						}
					}
					ReturnResult(resp);
				} catch (...) {
					delete resp;
					throw;
				}
			}

		public:
			ServerListQuery(MainScreenHelper *owner) : owner(owner) {}

			virtual void Run() {
				try {
					CURL *cHandle = curl_easy_init();
					if (cHandle) {
						try {
							curl_easy_setopt(cHandle, CURLOPT_USERAGENT, OpenSpades_VER_STR);
							curl_easy_setopt(cHandle, CURLOPT_URL, cl_serverListUrl.CString());
							curl_easy_setopt(cHandle, CURLOPT_WRITEFUNCTION,
							                 &ServerListQuery::curlWriteCallback);
							curl_easy_setopt(cHandle, CURLOPT_WRITEDATA, this);
							if (0 == curl_easy_perform(cHandle)) {

								ProcessResponse();

							} else {
								SPRaise("HTTP request error.");
							}
							curl_easy_cleanup(cHandle);
						} catch (...) {
							curl_easy_cleanup(cHandle);
							throw;
						}
					} else {
						SPRaise("Failed to create cURL object.");
					}
				} catch (std::exception &ex) {
					MainScreenServerList *lst = new MainScreenServerList();
					lst->message = ex.what();
					ReturnResult(lst);
				} catch (...) {
					MainScreenServerList *lst = new MainScreenServerList();
					lst->message = "Unknown error.";
					ReturnResult(lst);
				}
			}
		};

		MainScreenHelper::MainScreenHelper(MainScreen *scr)
		    : mainScreen(scr), result(NULL), newResult(NULL), query(NULL) {
			SPADES_MARK_FUNCTION();
			LoadFavorites();
		}

		MainScreenHelper::~MainScreenHelper() {
			SPADES_MARK_FUNCTION();
			if (query) {
				query->MarkForAutoDeletion();
			}
			delete result;
			delete newResult;
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

			IStream *fobj = spades::FileManager::OpenForWriting(FAVORITE_PATH);
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
				auto entry = std::find_if(
				  result->list.begin(), result->list.end(),
				  [&](MainScreenServerItem *entry) { return entry->GetAddress() == ip; });
				if (entry != result->list.end()) {
					(*entry)->SetFavorite(favorite);
				}
			}
		}

		bool MainScreenHelper::PollServerListState() {
			SPADES_MARK_FUNCTION();
			AutoLocker lock(&newResultArrayLock);
			if (newResult) {
				delete result;
				result = newResult;
				newResult = NULL;
				query->MarkForAutoDeletion();
				query = NULL;
				return true;
			}
			return false;
		}

		void MainScreenHelper::StartQuery() {
			if (query) {
				// ongoing
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

		struct serverSorter {
			int type;
			bool order;
			serverSorter(int type_, bool order_) : type(type_), order(order_) { ; }
			bool operator()(MainScreenServerItem *x, MainScreenServerItem *y) const {
				if (x->IsFavorite() && !y->IsFavorite()) {
					return true;
				} else if (!x->IsFavorite() && y->IsFavorite()) {
					return false;
				}

				if (order) {
					MainScreenServerItem *t = x;
					x = y;
					y = t;
				}
				switch (type) {
					default:
					case 0: return asInt(x->GetPing(), y->GetPing());
					case 1: return asInt(x->GetNumPlayers(), y->GetNumPlayers());
					case 2: return asString(x->GetName(), y->GetName());
					case 3: return asString(x->GetMapName(), y->GetMapName());
					case 4: return asString(x->GetGameMode(), y->GetGameMode());
					case 5: return asString(x->GetProtocol(), y->GetProtocol());
					case 6: return asString(x->GetCountry(), y->GetCountry());
				}
			}
			bool asInt(int x, int y) const {
				if (x == y)
					return false;
				return (x < y);
			}
			bool asString(const std::string &x, const std::string &y) const {
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
			}
			static bool ciCharLess(char c1, char c2) {
				return (std::tolower(static_cast<char>(c1)) < std::tolower(static_cast<char>(c1)));
			}
		};

		CScriptArray *MainScreenHelper::GetServerList(std::string sortKey, bool descending) {
			if (result == NULL) {
				return NULL;
			}

			std::vector<MainScreenServerItem *> lst = result->list;
			if (lst.empty())
				return NULL;

			int sortKeyId = -1;
			if (sortKey == "Ping") {
				sortKeyId = 0;
			} else if (sortKey == "NumPlayers") {
				sortKeyId = 1;
			} else if (sortKey == "Name") {
				sortKeyId = 2;
			} else if (sortKey == "MapName") {
				sortKeyId = 3;
			} else if (sortKey == "GameMode") {
				sortKeyId = 4;
			} else if (sortKey == "Protocol") {
				sortKeyId = 5;
			} else if (sortKey == "Country") {
				sortKeyId = 6;
			}
			if (sortKeyId != -1) {
				std::sort(lst.begin(), lst.end(), serverSorter(sortKeyId, descending));
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

		PackageUpdateManager& MainScreenHelper::GetPackageUpdateManager() {
			return PackageUpdateManager::GetInstance();
		}

		MainScreenServerList::~MainScreenServerList() {
			for (size_t i = 0; i < list.size(); i++)
				list[i]->Release();
		}

		MainScreenServerItem::MainScreenServerItem(Serveritem *item, bool favorite) {
			SPADES_MARK_FUNCTION();
			name = item->Name();
			address = item->Ip();
			mapName = item->Map();
			gameMode = item->GameMode();
			country = item->Country();
			protocol = item->Version();
			ping = item->Ping();
			numPlayers = item->Players();
			maxPlayers = item->MaxPlayers();
			this->favorite = favorite;
		}

		MainScreenServerItem::~MainScreenServerItem() { SPADES_MARK_FUNCTION(); }
	}
}
