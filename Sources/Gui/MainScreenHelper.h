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

#pragma once

#include <unordered_set>
#include <vector>

#include <Core/Mutex.h>
#include <Core/RefCountedObject.h>
#include <ScriptBindings/ScriptManager.h>

#include <AngelScript/addons/scriptarray.h>

namespace spades {
	class Serveritem;
	class PackageUpdateManager;
	namespace gui {
		class MainScreen;

		class MainScreenServerItem : public RefCountedObject {
			std::string name;
			std::string address;
			std::string mapName;
			std::string gameMode;
			std::string country;
			std::string protocol;
			int ping;
			int numPlayers;
			int maxPlayers;
			bool favorite;

		protected:
			~MainScreenServerItem();

		public:
			MainScreenServerItem(Serveritem *, bool favorite);

			std::string GetName() { return name; }
			std::string GetAddress() { return address; }
			std::string GetMapName() { return mapName; }
			std::string GetGameMode() { return gameMode; }
			std::string GetCountry() { return country; }
			std::string GetProtocol() { return protocol; }
			int GetPing() { return ping; }
			int GetNumPlayers() { return numPlayers; }
			int GetMaxPlayers() { return maxPlayers; }
			bool IsFavorite() { return favorite; }
			void SetFavorite(bool favorite) { this->favorite = favorite; }
		};

		struct MainScreenServerList {
			std::vector<MainScreenServerItem *> list;
			std::string message;

			~MainScreenServerList();
		};

		class MainScreenHelper : public RefCountedObject {
			friend class MainScreen;
			class ServerListQuery;

			MainScreen *mainScreen;
			MainScreenServerList *result;
			MainScreenServerList *volatile newResult;
			Mutex newResultArrayLock;
			ServerListQuery *volatile query;
			std::string errorMessage;
			std::unordered_set<std::string> favorites;

		protected:
			~MainScreenHelper();

		public:
			MainScreenHelper(MainScreen *scr);
			void MainScreenDestroyed();

			void LoadFavorites();
			void SaveFavorites();
			void SetServerFavorite(std::string ip, bool favorite);

			bool PollServerListState();
			void StartQuery();
			CScriptArray *GetServerList(std::string sortKey, bool descending);
			std::string GetServerListQueryMessage();

			std::string ConnectServer(std::string hostname, int protocolVersion);
			std::string GetPendingErrorMessage();

			std::string GetCredits();

			PackageUpdateManager& GetPackageUpdateManager();
		};
	}
}
