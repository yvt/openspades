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

#include <string>
#include <map>
#include <vector>

namespace spades {
	class Settings {
		struct Item {
			std::string name;
			std::string string;
			float value;
			int intValue;
			
			std::string defaultValue;
			std::string desc;
			
			void Set(const std::string&);
			void Set(int);
			void Set(float);
		};
		std::map<std::string, Item *> items;
		Settings();
		
		Item *GetItem(const std::string& name,
					  const std::string& def,
					  const std::string& desc);
		
	public:
		static Settings *GetInstance();
		
		
		class ItemHandle {
			Item *item;
		public:
			ItemHandle(const std::string& name,
					   const std::string& def = std::string(),
					   const std::string& desc = std::string());
			void operator =(const std::string&);
			void operator =(int);
			void operator =(float);
			operator std::string();
			operator float();
			operator int();
			operator bool();
			const char *CString();
			
			std::string GetDescription();
		};
		
		void Flush();
		std::vector<std::string> GetAllItemNames();
		
	};
	/*
	template<const char *name, const char *def>
	class Setting: public Settings::ItemHandle {
	public:
		Setting(): Settings::ItemHandle(name, def, desc){
		}
	};*/
	
	static bool operator ==(const std::string& str, Settings::ItemHandle& handle) {
		return str == (std::string)handle;
	}
	
#define SPADES_SETTING(name, def) static spades::Settings::ItemHandle name ( # name , def, "" )
	
#define SPADES_SETTING2(name, def, desc) static spades::Settings::ItemHandle name ( # name , def, desc )
	
}