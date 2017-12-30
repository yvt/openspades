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

#include "ScriptManager.h"
#include <Core/Settings.h>
#include <Core/RefCountedObject.h>
#include <Core/ThreadLocalStorage.h>
#include <unordered_map>

namespace spades {

	namespace
	{
		ThreadLocalStorage<bool> writeAllowed;

		// SettingItemDescriptor supplied to ItemHandle must have the static storage duration
		std::unordered_map<std::string, const SettingItemDescriptor *>
		settingItemDescriptors;

		const SettingItemDescriptor *MakeSettingItemDescriptor
		(const std::string &name, const std::string &defaultValue)
		{
			auto it = settingItemDescriptors.find(name);
			if (it != settingItemDescriptors.end()) {
				return it->second;
			}
			auto *descriptor = new SettingItemDescriptor(defaultValue, SettingItemFlags::None);
			settingItemDescriptors.insert(make_pair(name, descriptor));
			return descriptor;
		}
	}

	void MaskConfigUpdateByScript(bool disabled)
	{
		if (!writeAllowed) {
			writeAllowed = new bool;
		}
		*writeAllowed = !disabled;
	}


	class ConfigRegistrar: public ScriptObjectRegistrar {


	public:
		ConfigRegistrar():
		ScriptObjectRegistrar("Config") {}

		class ConfigItem: public RefCountedObject {
			Settings::ItemHandle handle;
		public:
			ConfigItem(const std::string& name, const std::string& defaultValue):
			handle(name, MakeSettingItemDescriptor(name, defaultValue)){
			}
			ConfigItem(const std::string& name):
			handle(name, nullptr){
			}

			static ConfigItem *Construct(const std::string& name, const std::string& defaultValue) {
				return new ConfigItem(name, defaultValue);
			}
			static ConfigItem *Construct(const std::string& name) {
				return new ConfigItem(name);
			}

			ConfigItem *operator =(float fv) {
				if (!writeAllowed || !*writeAllowed) {
					return this;
				}
				handle = fv;
				AddRef();
				return this;
			}
			ConfigItem *operator =(int v) {
				if (!writeAllowed || !*writeAllowed) {
					return this;
				}

				handle = v;
				AddRef();
				return this;
			}
			ConfigItem *operator =(const std::string& v) {
				if (!writeAllowed || !*writeAllowed) {
					return this;
				}

				handle = v;
				AddRef();
				return this;
			}
			void SetValue(float v) {
				handle = v;
			}
			void SetValue(int v) {
				handle = v;
			}
			void SetValue(const std::string& v) {
				handle = v;
			}
			float GetFloatValue() {
				return (float)handle;
			}
			int GetIntValue() {
				return (int)handle;
			}
			bool GetBoolValue() {
				return (bool)handle;
			}
			std::string GetStringValue() {
				return (std::string)handle;
			}
			std::string GetDefaultValue() {
				return handle.GetDescriptor().defaultValue;
			}
			bool IsUnknown() {
				return handle.IsUnknown();
			}
		};

		static CScriptArray *GetAllConfigNames() {
			auto *ctx = asGetActiveContext();
			auto *engine = ctx->GetEngine();
			auto *arrayType = engine->GetTypeInfoByDecl("array<string>");
			auto *array = CScriptArray::Create(arrayType);
			auto names = Settings::GetInstance()->GetAllItemNames();
			array->Resize(static_cast<asUINT>(names.size()));
			for(std::size_t i = 0; i < names.size(); i++) {
				reinterpret_cast<std::string *>(array->At(static_cast<asUINT>(i)))->assign(names[i]);
			}
			return array;
		}

		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("ConfigItem",
												0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour("ConfigItem",
													 asBEHAVE_ADDREF,
													 "void f()",
													 asMETHOD(ConfigItem, AddRef),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("ConfigItem",
													 asBEHAVE_RELEASE,
													 "void f()",
													 asMETHOD(ConfigItem, Release),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("ConfigItem",
													 asBEHAVE_FACTORY,
													 "ConfigItem @f(const string& in)",
													 asFUNCTIONPR(ConfigItem::Construct, (const std::string&), ConfigItem *),
													 asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("ConfigItem",
													 asBEHAVE_FACTORY,
													 "ConfigItem @f(const string& in, const string& in)",
													 asFUNCTIONPR(ConfigItem::Construct, (const std::string&, const std::string&), ConfigItem *),
													 asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "ConfigItem@ opAssign(float)",
												  asMETHODPR(ConfigItem, operator=, (float), ConfigItem *),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "ConfigItem@ opAssign(int)",
												  asMETHODPR(ConfigItem, operator=, (int), ConfigItem *),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "ConfigItem@ opAssign(const string& in)",
												  asMETHODPR(ConfigItem, operator=, (const std::string&), ConfigItem *),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "void set_IntValue(int)",
												  asMETHODPR(ConfigItem, SetValue, (int), void),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "void set_FloatValue(float)",
												  asMETHODPR(ConfigItem, SetValue, (float), void),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "void set_StringValue(const string& in)",
												  asMETHODPR(ConfigItem, SetValue, (const std::string&), void),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "int get_IntValue()",
												  asMETHOD(ConfigItem, GetIntValue),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "float get_FloatValue()",
												  asMETHOD(ConfigItem, GetFloatValue),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "string get_StringValue()",
												  asMETHOD(ConfigItem, GetStringValue),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "string get_DefaultValue()",
												  asMETHOD(ConfigItem, GetDefaultValue),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("ConfigItem",
												  "bool get_IsUnknown()",
												  asMETHOD(ConfigItem, IsUnknown),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterGlobalFunction("array<string>@ GetAllConfigNames()",
												  asFUNCTION(GetAllConfigNames),
												  asCALL_CDECL);
					manager->CheckError(r);
					break;
				default:
					break;
			}
		}
	};

	static ConfigRegistrar registrar;

}

