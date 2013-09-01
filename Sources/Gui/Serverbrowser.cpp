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

#include <OpenSpades.h>
#include "Serverbrowser.h"
#include <Core/Settings.h>
#include <curl/curl.h>
#include <json/json.h>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <sstream>
#include <cctype>
#include <algorithm>

SPADES_SETTING2(cg_protocolVersion, "", "The protocol version to use, 3 = 0.75, 4 = 0.76");
SPADES_SETTING(cg_serverlistFilter, "31");
SPADES_SETTING(cg_serverlistSort, "0");

#define SERVICE_URL	"http://services.buildandshoot.com/serverlist.json"

#define COLUMN_CHAR	"\t"

namespace spades
{


Serveritem::Serveritem( const std::string& name, const std::string& ip, const std::string& map, const std::string& gameMode, const std::string& country, const std::string& version, int ping, int players, int maxPlayers )
: mName(name), mIp(ip), mMap(map), mGameMode(gameMode), mCountry(country), mVersion(version), mPing(ping), mPlayers(players), mMaxPlayers(maxPlayers)
{
}

Serveritem* Serveritem::create( Json::Value& val )
{
	Serveritem* item = NULL;
	if( val.type() == Json::objectValue ) {
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
		item = new Serveritem( name, ip, map, gameMode, country, version, ping, players, maxPlayers );
	}
	return item;
}


std::string Serveritem::toRow()
{
	std::stringstream ss;
	ss << "@." << mPing << COLUMN_CHAR
		<< mPlayers << "/" << mMaxPlayers << COLUMN_CHAR
		<< mName << COLUMN_CHAR
		<< mMap << COLUMN_CHAR
		<< mGameMode << COLUMN_CHAR
		<< mVersion << COLUMN_CHAR
		<< mCountry;
	return ss.str();
}

#define HEAD_FMT	"@b"
#define SORT_FMT	"@C216"

std::string Serveritem::rowHeaders(int sort)
{
	std::stringstream ss;
	ss << (sort == 0 ? SORT_FMT : "") << HEAD_FMT "Ping" COLUMN_CHAR
		<< (sort == 1 ? SORT_FMT : "") << HEAD_FMT "Players" COLUMN_CHAR
		<< (sort == 2 ? SORT_FMT : "") << HEAD_FMT "Name" COLUMN_CHAR
		<< (sort == 3 ? SORT_FMT : "") << HEAD_FMT "Map" COLUMN_CHAR
		<< (sort == 4 ? SORT_FMT : "") << HEAD_FMT "Gamemode" COLUMN_CHAR
		<< (sort == 5 ? SORT_FMT : "") << HEAD_FMT "Version" COLUMN_CHAR
		<< (sort == 6 ? SORT_FMT : "") << HEAD_FMT "Loc";
	return ss.str();
}

int size_Array[] = { 40, 60, 250, 80, 60, 50, 40, 0 };

int* Serveritem::colSizes()
{
	return size_Array;
}

struct serverSorter {
	int type, order;
	serverSorter( int type_, int order_ ) : type(type_), order(order_) {;}
	bool operator() ( Serveritem* x, Serveritem* y ) const
	{
		if( order ) {
			Serveritem* t = x;
			x = y;
			y = t;
		}
		switch( type ) {
			default:
			case 0:	return asInt( x->Ping(), y->Ping() );
			case 1:	return asInt( x->Players(), y->Players() );
			case 2:	return asString( x->Name(), y->Name() );
			case 3:	return asString( x->Map(), y->Map() );
			case 4: return asString( x->GameMode(), y->GameMode() );
			case 5: return asString( x->Version(), y->Version() );
			case 6: return asString( x->Country(), y->Country() );
		}
	}
	bool asInt( int x, int y ) const
	{
		if( x == y ) return false;
		return (x<y);
	}
	bool asString( const std::string& x, const std::string& y ) const
	{
		std::string::size_type t = 0;
		for( t = 0; t < x.length() && t < y.length(); ++ t ) {
			int xx = std::tolower(x[t]);
			int yy = std::tolower(y[t]);
			if( xx != yy ) {
				return xx < yy;
			}
		}
		if( x.length() == y.length() ) {
			return false;
		}
		return x.length() < y.length();
	}
	static bool ciCharLess( char c1, char c2 )
	{
		return (std::tolower( static_cast<char>( c1 ) ) < std::tolower( static_cast<char>( c1 ) ));
	}
};



Serverbrowser::Serverbrowser( Fl_Browser* box )
: mStopRequested(false), mBrowser( box ), mSort(0), mSortOrder(0)
{
	mFlags = static_cast<ServerFilter::Flags>((int)cg_serverlistFilter);
	mSortOrder = ((int)cg_serverlistSort & 0x4000) ? 1 : 0;
	mSort = ((int)cg_serverlistSort) & 0xfff;
	curl_global_init( CURL_GLOBAL_ALL );
}

void Serverbrowser::Run()
{
	CURL* cHandle = curl_easy_init();
	mBrowser->clear();
	if( cHandle ) {
		mBrowser->add( "Fetching servers, please wait..." );
		mBuffer = "";
		curl_easy_setopt( cHandle, CURLOPT_USERAGENT, OpenSpades_VER_STR );
		curl_easy_setopt( cHandle, CURLOPT_URL, SERVICE_URL );
		curl_easy_setopt( cHandle, CURLOPT_WRITEFUNCTION, &Serverbrowser::curlWriteCallback );
		curl_easy_setopt( cHandle, CURLOPT_WRITEDATA, this );
		if( 0 == curl_easy_perform( cHandle ) ) {
			parse();
			refreshList();
		} else {
			mBrowser->add( "Sorry, couldnt fetch servers..." );
		}
		curl_easy_cleanup( cHandle );
	}
}

size_t Serverbrowser::curlWriteCallback( void *ptr, size_t size, size_t nmemb, Serverbrowser* browser )
{
	size_t dataSize = size * nmemb;
	return browser->writeCallback( ptr, dataSize );
}

size_t Serverbrowser::writeCallback( void *ptr, size_t size )
{
	if( mStopRequested ) {
		return 0;	//abort.
	}
	mBuffer.append( reinterpret_cast<char*>(ptr), size );
	return size;
}

void Serverbrowser::parse()
{
	Json::Reader reader;
	Json::Value root;
	if( reader.parse( mBuffer, root, false ) ) {
		for( Json::Value::iterator it = root.begin(); it != root.end(); ++it ) {
			Json::Value &obj = *it;
			Serveritem* srv = Serveritem::create( obj );
			if( srv ) {
				mServers.push_back( srv );
			}
		}
	}
}

bool Serverbrowser::hasFlag( ServerFilter::Flags flag )
{
	return (mFlags & flag) != 0;
}

void Serverbrowser::refreshList()
{
	std::sort( mServers.begin(), mServers.end(), serverSorter( mSort, mSortOrder ) );
	mBrowser->clear();
	mBrowser->column_widths( Serveritem::colSizes() );
	mBrowser->column_char( COLUMN_CHAR[0] );
	mBrowser->add( Serveritem::rowHeaders( mSort ).c_str() );
	for( size_t n = 0; n < mServers.size(); ++n ) {
		Serveritem* i = mServers[n];
		if( (i->Players() == 0 && !hasFlag(ServerFilter::flt_Empty)) ||
			(i->Players() == i->MaxPlayers() && !hasFlag(ServerFilter::flt_Full) ) ) {
			continue;
		}
		if( "0.75" == i->Version() ) {
			if( !hasFlag( ServerFilter::flt_Ver075 ) ) {
				continue;
			}
		} else if( "0.76" == i->Version() ) {
			if( !hasFlag( ServerFilter::flt_Ver076 ) ) {
				continue;
			}
		} else {
			if( !hasFlag( ServerFilter::flt_VerOther ) ) {
				continue;
			}
		}
		mBrowser->add( i->toRow().c_str(), i );
	}
}

void Serverbrowser::onSelection( void* ptr, Fl_Input* input )
{
	Serveritem* serverItem = static_cast<Serveritem*>(ptr);
	if( serverItem ) {
		std::string ip = serverItem->Ip();
		input->value( ip.c_str() );
		if( serverItem->Version() == "0.75" ) {
			cg_protocolVersion = "3";
		} else if( serverItem->Version() == "0.76" ) {
			cg_protocolVersion = "4";
		}
	}
}

void Serverbrowser::onHeaderClick( int x )
{
	int* p = Serveritem::colSizes();
	int num = 0;
	while( p[num] && x > p[num] ) {
		x -= p[num++];
	}
	if( p[num] ) {
		if( num == mSort ) {
			mSortOrder ^= 1;
		} else {
			mSort = num;
			mSortOrder = 0;
		}
		cg_serverlistSort = (mSort & 0xfff) | (mSortOrder ? 0x4000 : 0);
		refreshList();
	}
}

void Serverbrowser::setFilter( ServerFilter::Flags newFlags )
{
	mFlags = newFlags;
	cg_serverlistFilter = static_cast<int>(newFlags);
}

}; //namespace spades
