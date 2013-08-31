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
#include <curl/curl.h>
#include <json/json.h>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <sstream>

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
	ss << mPing << COLUMN_CHAR
		<< mPlayers << "/" << mMaxPlayers << COLUMN_CHAR
		<< mName << COLUMN_CHAR
		<< mMap << COLUMN_CHAR
		<< mGameMode << COLUMN_CHAR
		<< mVersion << COLUMN_CHAR
		<< mCountry;
	return ss.str();
}

std::string Serveritem::rowHeaders()
{
	return "Ping" COLUMN_CHAR
		"Players" COLUMN_CHAR
		"Name" COLUMN_CHAR
		"Map" COLUMN_CHAR
		"Gamemode" COLUMN_CHAR
		"Version" COLUMN_CHAR
		"Loc";
}

int size_Array[] = { 40, 60, 250, 80, 60, 50, 40, 0 };

int* Serveritem::colSizes()
{
	return size_Array;
}



Serverbrowser::Serverbrowser( Fl_Browser* box )
: mBrowser( box )
{
	curl_global_init( CURL_GLOBAL_ALL );
	mBrowser->clear();
	mBrowser->add( "Fetching servers, please wait..." );
}

void Serverbrowser::Run()
{
	CURL* cHandle = curl_easy_init();
	if( cHandle ) {
		mBuffer = "";
		curl_easy_setopt( cHandle, CURLOPT_USERAGENT, OpenSpades_VER_STR );
		curl_easy_setopt( cHandle, CURLOPT_URL, SERVICE_URL );
		curl_easy_setopt( cHandle, CURLOPT_WRITEFUNCTION, &Serverbrowser::curlWriteCallback );
		curl_easy_setopt( cHandle, CURLOPT_WRITEDATA, this );
		if( 0 == curl_easy_perform( cHandle ) ) {
			parse();
			refreshList();
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
	if( false ) {
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
	//std::sort(bla)
	mBrowser->clear();
	mBrowser->column_widths( Serveritem::colSizes() );
	mBrowser->column_char( COLUMN_CHAR[0] );
	mBrowser->add( Serveritem::rowHeaders().c_str() );
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
	}
}

void Serverbrowser::setFilter( ServerFilter::Flags newFlags )
{
	mFlags = newFlags;
}

}; //namespace spades
