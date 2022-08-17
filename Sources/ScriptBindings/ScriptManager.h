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

#include <AngelScript/include/angelscript.h>
#include <AngelScript/add_on/scriptany/scriptany.h>
#include <AngelScript/add_on/scriptarray/scriptarray.h>
#include <AngelScript/add_on/scriptbuilder/scriptbuilder.h>
#include <AngelScript/add_on/scriptdictionary/scriptdictionary.h>
#include <AngelScript/add_on/scripthandle/scripthandle.h>
#include <AngelScript/add_on/scripthelper/scripthelper.h>
#include <AngelScript/add_on/scriptmath/scriptmath.h>
#include <AngelScript/add_on/scriptmath/scriptmathcomplex.h>
#include <AngelScript/add_on/scriptstdstring/scriptstdstring.h>
#include <AngelScript/add_on/weakref/weakref.h>
#include <list>
#include <mutex>

namespace spades {

	class ScriptContextHandle;

	class ScriptManager {
		friend class ScriptContextHandle;
		struct Context {
			asIScriptContext *obj;
			int refCount;
		};
		std::recursive_mutex contextMutex;
		std::list<Context *> contextFreeList;

		asIScriptEngine *engine;

		ScriptManager();
		~ScriptManager();
	public:
		static ScriptManager *GetInstance();

		static void CheckError(int);

		asIScriptEngine *GetEngine() const { return engine; }

		ScriptContextHandle GetContext();
	};

	class ScriptContextUtils {
		asIScriptContext *context;

		void appendLocation( std::stringstream& ss, asIScriptFunction* func, const char *secName, int line, int column );
	public:
		ScriptContextUtils();
		ScriptContextUtils(asIScriptContext *);
		void ExecuteChecked();
		void SetNativeException(const std::exception&);
	};

	class ScriptContextHandle{
		ScriptManager *manager;
		ScriptManager::Context *obj;

		void Release();
	public:
		ScriptContextHandle();
		ScriptContextHandle(ScriptManager::Context *,
							ScriptManager *manager);
		ScriptContextHandle(const ScriptContextHandle&);
		~ScriptContextHandle();
		void operator =(const ScriptContextHandle&);
		asIScriptContext *GetContext() const;
		asIScriptContext *operator ->() const;

		ScriptManager *GetManager() const { return manager; }

		void ExecuteChecked();
	};

	class ScriptObjectRegistrar {
	public:
		enum Phase {
			PhaseObjectType,
			PhaseGlobalFunction,
			PhaseObjectMember,
			PhaseCount
		};
		ScriptObjectRegistrar(const std::string& name);
		virtual void Register(ScriptManager *manager, Phase) = 0;

		static void RegisterOne(const std::string& name,
								ScriptManager *manager,
								Phase);
		static void RegisterAll(ScriptManager *manager, Phase);

	private:
		bool phaseDone[PhaseCount];
		std::string name;
	};

}
