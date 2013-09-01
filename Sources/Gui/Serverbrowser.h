/*
 Copyright (c) 2013 learn_more
 
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

#include <Core/Thread.h>
#include <Core/DynamicMemoryStream.h>

//lm: i prefer to keep the headers lean and mean, only include what is absolutely needed, and forward declare the rest.
namespace Json { class Value; };
class Fl_Browser;
class Fl_Input;

namespace spades
{

class Serveritem
{
	//NetClient::Connect
	std::string mName, mIp, mMap, mGameMode;
	std::string mCountry, mVersion;
	int mPing, mPlayers, mMaxPlayers;

	Serveritem( const std::string& name, const std::string& ip, const std::string& map, const std::string& gameMode, const std::string& country, const std::string& version,
		int ping, int players, int maxPlayers );
public:
	static Serveritem* create( Json::Value& val );
	static std::string rowHeaders( int sort );
	static int* colSizes();

	std::string toRow();

	inline const std::string& Name() const { return mName; }
	inline const std::string& Ip() const { return mIp; }
	inline const std::string& Map() const { return mMap; }
	inline const std::string& GameMode() const { return mGameMode; }
	inline const std::string& Country() const { return mCountry; }
	inline const std::string& Version() const { return mVersion; }
	inline int Ping() const { return mPing; }
	inline int Players() const { return mPlayers; }
	inline int MaxPlayers() const { return mMaxPlayers; }
};

namespace ServerFilter
{
	enum Flags {
		flt_None = 0,
		flt_Empty = (1<<0),
		flt_Full = (1<<1),
		flt_Ver075 = (1<<2),
		flt_Ver076 = (1<<3),
		flt_VerOther = (1<<4)
	};
};



class Serverbrowser : public Thread
{
	bool mStopRequested;
	std::string mBuffer;
	std::vector<Serveritem*> mServers;
	Fl_Browser* mBrowser;
	ServerFilter::Flags mFlags;
	int mSort, mSortOrder;

	virtual void Run();
	static size_t curlWriteCallback( void *ptr, size_t size, size_t nmemb, Serverbrowser* data );
	size_t writeCallback( void *ptr, size_t size );

	void parse();
	bool hasFlag( ServerFilter::Flags flag );

public:
	Serverbrowser( Fl_Browser* box );
	void stopReading() { mStopRequested = true; }
	void onSelection( void* ptr, Fl_Input* input );
	void onHeaderClick( int x );
	void setFilter( ServerFilter::Flags newFlags );
	inline ServerFilter::Flags Filter() { return mFlags; }
	void refreshList();
};

}; //namespace spades

inline static spades::ServerFilter::Flags& operator |= (spades::ServerFilter::Flags& a, const spades::ServerFilter::Flags b)
{
	a = static_cast<spades::ServerFilter::Flags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
	return a;
}


