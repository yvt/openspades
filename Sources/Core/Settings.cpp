//
//  Settings.cpp
//  OpenSpades
//
//  Created by yvt on 7/21/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include <stdio.h>
#include "Settings.h"
#include <FL/Fl_Preferences.H>
#include "../Core/Debug.h"
#include <stdlib.h>

namespace spades {
	
	// FIXME: provide application local preference mechanism?
	
	static Settings *instance = NULL;
	static Fl_Preferences *pref = NULL;
	
	Settings *Settings::GetInstance() {
		if(!instance)
			instance = new Settings();
		return instance;
	}
	
	Settings::Settings() {
		pref = new Fl_Preferences(Fl_Preferences::USER,
								  "yvt.jp",
								  "OpenSpades");
	}
	
	void Settings::Flush() {
		pref->flush();
	}
	
	std::vector<std::string> Settings::GetAllItemNames() {
		std::vector<std::string> names;
		std::map<std::string, Item *>::iterator it;
		for(it = items.begin(); it != items.end(); it++){
			names.push_back(it->second->name);
		}
		return names;
	}

	Settings::Item *Settings::GetItem(const std::string &name,
									  const std::string& def,
									  const std::string& desc){
		std::map<std::string, Item *>::iterator it;
		it = items.find(name);
		if(it == items.end()){
			Item *item = new Item();
			item->name = name;
			
			char buf[2048];
			buf[2047] = 0;
			SPAssert(pref != NULL);
			pref->get(name.c_str(), buf, def.c_str(), 2047);
			SPAssert(buf);
			
			item->string = buf;
			
			item->value = atof(item->string.c_str());
			item->intValue = atoi(item->string.c_str());
			
			item->desc = desc;
			item->defaultValue = def;
			
			items[name] = item;
			return item;
		}
		return it->second;
	}
	
	void Settings::Item::Set(const std::string &str) {
		string = str;
		value = atof(str.c_str());
		intValue = atoi(str.c_str());
		
		pref->set(name.c_str(), string.c_str());
	}
	
	void Settings::Item::Set(int v) {
		char buf[256];
		sprintf(buf, "%d", v);
		string = buf;
		intValue = v;
		value = (float)v;
		
		pref->set(name.c_str(), string.c_str());
	}
	
	void Settings::Item::Set(float v){
		char buf[256];
		sprintf(buf, "%f", v);
		string = buf;
		intValue = (int)v;
		value = v;
		
		pref->set(name.c_str(), string.c_str());
	}
	
	Settings::ItemHandle::ItemHandle(const std::string& name,
									 const std::string& def,
									 const std::string& desc){
		item = Settings::GetInstance()->GetItem(name, def, desc);
		if(def.size() > 0 && item->defaultValue.size() == 0){
			item->defaultValue = def;
		}
		if(desc.size() > 0 && item->desc.size() == 0){
			item->desc = desc;
		}
		if(item->defaultValue != def && def.size() > 0){
			fprintf(stderr, "WARNING: setting '%s' has multiple default values ('%s', '%s')\n",
					name.c_str(), def.c_str(), item->defaultValue.c_str());
		}
	}
	
	void Settings::ItemHandle::operator =(const std::string& value){
		item->Set(value);
	}
	void Settings::ItemHandle::operator =(int value){
		item->Set(value);
	}
	void Settings::ItemHandle::operator =(float value){
		item->Set(value);
	}
	Settings::ItemHandle::operator std::string() {
		return item->string;
	}
	Settings::ItemHandle::operator int() {
		return item->intValue;
	}
	Settings::ItemHandle::operator float() {
		return item->value;
	}
	Settings::ItemHandle::operator bool() {
		return item->intValue != 0;
	}
	const char *Settings::ItemHandle::CString() {
		return item->string.c_str();
	}
	std::string Settings::ItemHandle::GetDescription() {
		return item->desc;
	}
	
	
}
