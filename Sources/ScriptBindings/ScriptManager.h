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
#include <AngelScript/addons/scriptany.h>
#include <AngelScript/addons/scriptarray.h>
#include <AngelScript/addons/scriptbuilder.h>
#include <AngelScript/addons/scriptdictionary.h>
#include <AngelScript/addons/scripthandle.h>
#include <AngelScript/addons/scripthelper.h>
#include <AngelScript/addons/scriptmath.h>
#include <AngelScript/addons/scriptmathcomplex.h>
#include <AngelScript/addons/scriptstdstring.h>
#include <AngelScript/addons/weakref.h>

namespace spades {
	
	class ScriptManager {
		
		asIScriptEngine *engine;
		
		ScriptManager();
		~ScriptManager();
	public:
		static ScriptManager *GetInstance();
		
		static void CheckError(int);
		
		asIScriptEngine *GetEngine() const { return engine; }
	};
	
	class ScriptObjectRegistrar {
	public:
		enum Phase {
			PhaseObjectType,
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