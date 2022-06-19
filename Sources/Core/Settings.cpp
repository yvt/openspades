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

#include <cstdio>
#include <cstdlib>
#include <memory>

#include <Core/Debug.h>
#include "FltkPreferenceImporter.h"
#include "Settings.h"
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Math.h>

namespace spades {

#define CONFIGFILE "SPConfig.cfg"
	static Settings *instance = NULL;

	Settings *Settings::GetInstance() {
		if (!instance)
			instance = new Settings();
		return instance;
	}

	Settings::Settings() {
		SPADES_MARK_FUNCTION();
		loaded = false;
	}

	void Settings::Load() {
		SPADES_MARK_FUNCTION();

		// import Fltk preferences
		bool importedPref = false;
		{
			auto prefs = ImportFltkPreference();
			for (const auto &item : prefs) {
				auto *it = GetItem(item.first, nullptr);

				it->Set(item.second);
			}
			if (prefs.size() > 0)
				importedPref = true;
			// FIXME: remove legacy preference?
		}

		SPLog("Loading preferences from " CONFIGFILE);
		loaded = false;
		try {
			if (FileManager::FileExists(CONFIGFILE)) {
				SPLog(CONFIGFILE " found.");

				std::string text = FileManager::ReadAllBytes(CONFIGFILE);
				auto lines = SplitIntoLines(text);

				std::size_t line = 0;

				while (line < lines.size()) {
					auto &l = lines[line];
					{
						// remove comments
						auto pos = l.find('#');
						if (pos != std::string::npos) {
							l.resize(pos);
						}
					}

					std::size_t startPos = l.find_first_not_of(' ');
					if (startPos == std::string::npos) {
						// no contents in this line
						line++;
						continue;
					}

					auto lineBuf = l;
					std::size_t linePos = 0;

					auto tryDecodeHexDigit = [](char c, int &digit) -> bool {
						if (c >= '0' && c <= '9') {
							digit = c - '0';
							return true;
						} else if (c >= 'a' && c <= 'f') {
							digit = c - 'a' + 10;
							return true;
						} else if (c >= 'A' && c <= 'F') {
							digit = c - 'A' + 10;
							return true;
						} else {
							return false;
						}
					};

					auto readString = [&](bool stopAtColon) {
						std::string buffer;
						int digit1, digit2;
						while (linePos < lineBuf.size() && lineBuf[linePos] == ' ') {
							linePos++;
						}
						while (linePos < lineBuf.size()) {
							if (lineBuf[linePos] == '\\' && linePos + 1 == lineBuf.size() &&
							    line < lines.size() - 1) {
								// line continuation
								line++;
								lineBuf = lines[line];
								linePos = 0;
							} else if (lineBuf[linePos] == '\\' && linePos + 3 < lineBuf.size() &&
							           lineBuf[linePos + 1] == 'x' &&
							           tryDecodeHexDigit(lineBuf[linePos + 2], digit1) &&
							           tryDecodeHexDigit(lineBuf[linePos + 3], digit2)) {
								// hex
								char c = (digit1 << 4) | digit2;
								buffer += c;
								linePos += 3;
							} else if (lineBuf[linePos] == '\\' && linePos + 1 < lineBuf.size()) {
								// escape
								switch (lineBuf[linePos + 1]) {
									case 'n': buffer += '\n'; break;
									case 'r': buffer += '\r'; break;
									case 't': buffer += '\t'; break;
									default: buffer += lineBuf[linePos + 1]; break;
								}
								linePos += 2;
							} else if (lineBuf[linePos] == ':' && stopAtColon) {
								break;
							} else {
								// normal chars
								buffer += lineBuf[linePos];
								linePos++;
							}
						}
						return buffer;
					};

					std::string key = readString(true);
					if (linePos >= lineBuf.size()) {
						SPLog("Warning: no value provided for \"%s\"", key.c_str());
					}
					linePos++;

					std::string val = readString(false);
					auto *item = GetItem(key, nullptr);
					item->Set(val);

					line++;
				}

			} else {
				SPLog(CONFIGFILE " doesn't exist.");
			}

			if (importedPref) {
				SPLog("Legacy preference was imported. Removing the legacy pref file.");
				DeleteFltkPreference();
				Save();
			}

			loaded = true;
		} catch (const std::exception &ex) {
			SPLog("Failed to load preference: %s", ex.what());
			SPLog("Disabling saving preference.");
		}
	}

	void Settings::Save() {
		SPLog("Saving preferences to " CONFIGFILE);
		try {
			std::string buffer;
			buffer = "# OpenSpades config file\n"
			         "#\n"
			         "\n";

			int column = 0;

			auto emitContinuation = [&] {
				buffer += "\\\n";
				column = 0;
			};

			auto emitString = [&](const std::string &val, bool escapeColon) {
				std::size_t i = 0;
				while (i < val.size() && val[i] == ' ') {
					if (column > 78) {
						emitContinuation();
					}
					buffer += "\\ ";
					column += 2;
					i++;
				}
				while (i < val.size()) {
					if (column > 78) {
						emitContinuation();
					}
					unsigned char uc = static_cast<unsigned char>(val[i]);
					switch (val[i]) {
						case '\n':
							buffer += "\\n";
							column += 2;
							i++;
							break;
						case '\r':
							buffer += "\\r";
							column += 2;
							i++;
							break;
						case '\t':
							buffer += "\\t";
							column += 2;
							i++;
							break;
						default:
							std::size_t utf8charsize;
							GetCodePointFromUTF8String(val, i, &utf8charsize);

							if (val[i] == '#' ||                     // comment marker
							    (escapeColon && val[i] == ':') ||    // key/value split
							    uc < 0x20 ||                         // control char
							    (uc >= 0x80 && utf8charsize == 0) || // invalid UTF8
							    utf8charsize >=
							      5) { // valid UTF-8 but codepoint beyond BMP/SMP range
								static const char *s = "0123456789abcdef";
								buffer += "\\x";
								buffer += s[uc >> 4];
								buffer += s[uc & 15];
								column += 3;
								i++;
							} else {
								buffer.append(val, i, utf8charsize);
								column += utf8charsize;
								i += utf8charsize;
							}
							break;
					}
				}
			};

			for (const auto &item : items) {
				Item *itm = item.second;

				emitString(itm->name, true);
				buffer += ": ";
				column += 2;

				emitString(itm->string, false);

				buffer += "\n";
				column = 0;
			}

			std::unique_ptr<IStream> s(FileManager::OpenForWriting(CONFIGFILE));
			s->Write(buffer);

		} catch (const std::exception &ex) {
			SPLog("Failed to save preference: %s", ex.what());
		}
	}

	void Settings::Flush() {
		if (loaded) {
			SPLog("Saving preference to " CONFIGFILE);
			Save();
		} else {
			SPLog("Not saving preferences because loading preferences has failed.");
		}
	}

	std::vector<std::string> Settings::GetAllItemNames() {
		SPADES_MARK_FUNCTION();
		std::vector<std::string> names;
		std::map<std::string, Item *>::iterator it;
		for (it = items.begin(); it != items.end(); it++) {
			names.push_back(it->second->name);
		}
		return names;
	}

	Settings::Item *Settings::GetItem(const std::string &name,
	                                  const SettingItemDescriptor *descriptor) {
		SPADES_MARK_FUNCTION();
		std::map<std::string, Item *>::iterator it;
		Item *item;
		it = items.find(name);
		if (it == items.end()) {
			item = new Item();
			item->name = name;
			item->defaults = true;
			item->descriptor = nullptr;
			item->intValue = 0;
			item->value = 0.0f;

			items[name] = item;
		} else {
			item = it->second;
		}

		if (descriptor) {
			if (item->descriptor) {
				if (*item->descriptor != *descriptor) {
					SPLog("WARNING: setting '%s' has multiple descriptors", name.c_str());
				}
			} else {
				item->descriptor = descriptor;
				const std::string &defaultValue = descriptor->defaultValue;
				if (item->defaults) {
					item->value = static_cast<float>(atof(defaultValue.c_str()));
					item->intValue = atoi(defaultValue.c_str());
					item->string = defaultValue;
				}
			}
		}

		return item;
	}

	void Settings::Item::Load() {
		// no longer need to Load
	}

	void Settings::Item::Set(const std::string &str) {
		string = str;
		value = static_cast<float>(atof(str.c_str()));
		intValue = atoi(str.c_str());
		defaults = false;

		NotifyChange();
	}

	void Settings::Item::Set(int v) {
		SPADES_MARK_FUNCTION_DEBUG();
		char buf[256];
		sprintf(buf, "%d", v);
		string = buf;
		intValue = v;
		value = (float)v;
		defaults = false;

		NotifyChange();
	}

	void Settings::Item::Set(float v) {
		SPADES_MARK_FUNCTION_DEBUG();
		char buf[256];
		sprintf(buf, "%f", v);
		string = buf;
		intValue = (int)v;
		value = v;
		defaults = false;

		NotifyChange();
	}

	void Settings::Item::NotifyChange() {
		for (ISettingItemListener *listener : listeners) {
			listener->SettingChanged(name);
		}
	}

	Settings::ItemHandle::ItemHandle(const std::string &name,
	                                 const SettingItemDescriptor *descriptor) {
		SPADES_MARK_FUNCTION();

		item = Settings::GetInstance()->GetItem(name, descriptor);
	}

	void Settings::ItemHandle::operator=(const std::string &value) { item->Set(value); }
	void Settings::ItemHandle::operator=(int value) { item->Set(value); }
	void Settings::ItemHandle::operator=(float value) { item->Set(value); }
	bool Settings::ItemHandle::operator==(int value) {
		item->Load();
		return item->intValue == value;
	}
	bool Settings::ItemHandle::operator!=(int value) {
		item->Load();
		return item->intValue != value;
	}
	Settings::ItemHandle::operator std::string() {
		item->Load();
		return item->string;
	}
	Settings::ItemHandle::operator int() {
		item->Load();
		return item->intValue;
	}
	Settings::ItemHandle::operator float() {
		item->Load();
		return item->value;
	}
	Settings::ItemHandle::operator bool() {
		item->Load();
		return item->intValue != 0;
	}
	const char *Settings::ItemHandle::CString() {
		item->Load();
		return item->string.c_str();
	}
	void Settings::ItemHandle::AddListener(ISettingItemListener *listener) {
		auto &listeners = item->listeners;
		listeners.push_back(listener);
	}
	void Settings::ItemHandle::RemoveListener(ISettingItemListener *listener) {
		auto &listeners = item->listeners;
		auto it = std::find(listeners.begin(), listeners.end(), listener);
		if (it != listeners.end()) {
			listeners.erase(it);
		}
	}

	namespace {
		const SettingItemDescriptor defaultDescriptor{std::string(), SettingItemFlags::None};
	}

	const SettingItemDescriptor &Settings::ItemHandle::GetDescriptor() {
		return item->descriptor ? *item->descriptor : defaultDescriptor;
	}

	bool Settings::ItemHandle::IsUnknown() {
		return item->descriptor == nullptr;
	}
}
