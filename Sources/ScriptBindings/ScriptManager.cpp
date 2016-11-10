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
#include <Core/Debug.h>
#include <vector>
#include <sstream>
#include <Core/Exception.h>
#include <Core/AutoLocker.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>

namespace spades {
	
	ScriptManager *ScriptManager::GetInstance() {
		SPADES_MARK_FUNCTION_DEBUG();
		static ScriptManager *m = new ScriptManager();
		return m;
	}
	
	static void MessageCallback(const asSMessageInfo *msg, void *param){
		SPADES_MARK_FUNCTION();
		const char *type = "ERR ";
		if( msg->type == asMSGTYPE_WARNING )
			type = "WARN";
		else if( msg->type == asMSGTYPE_INFORMATION )
			type = "INFO";
		SPLog("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
	}
	
	class ScriptBuilder: public CScriptBuilder {
	public:
		ScriptBuilder(){
			
		}
	protected:
		int  LoadScriptSection(const char *filename) override {
			if(filename[0] != '/') {
				SPLog("Invalid script path detected: not starting with '/'");
				return -1;
			}
			
			// path validation should be done by filesystem...
			std::string data;
			try{
				std::string fn = "Scripts";
				fn += filename;
				data = FileManager::ReadAllBytes(fn.c_str());
			}catch(const std::exception& ex) {
				SPLog("Failed to include '%s':%s", filename, ex.what());
				return -1;
			}
			
			SPLog("Loading script '%s'", filename);
			return ProcessScriptSection(data.c_str(), (unsigned int)(data.length()), filename, 0);
		}
	};
	
	
	ScriptManager::ScriptManager() {
		SPADES_MARK_FUNCTION();
		
		SPLog("Creating script engine");
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		
		try{
			SPLog("Configuring");
			engine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, 1);
			engine->SetEngineProperty(asEP_DISALLOW_GLOBAL_VARS, 1);
			engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
			SPLog("Registering standard libray functions");
			RegisterScriptAny(engine);
			RegisterScriptWeakRef(engine);
			RegisterScriptHandle(engine);
			RegisterScriptMath(engine);
			RegisterScriptMathComplex(engine);
			RegisterScriptArray(engine, true);
			RegisterStdString(engine);
			RegisterStdStringUtils(engine);
			RegisterScriptDictionary(engine);
			
			SPLog("Registering APIs");
			engine->SetDefaultNamespace("");
			ScriptObjectRegistrar::RegisterAll(this, ScriptObjectRegistrar::PhaseObjectType);
			ScriptObjectRegistrar::RegisterAll(this, ScriptObjectRegistrar::PhaseGlobalFunction);
			ScriptObjectRegistrar::RegisterAll(this, ScriptObjectRegistrar::PhaseObjectMember);
			
			SPLog("Loading scripts");
			engine->SetDefaultNamespace("");
			ScriptBuilder builder;
			if(builder.StartNewModule(engine, "Client") < 0){
				SPRaise("Failed to create script module.");
			}
			builder.DefineWord("CLIENT");
			if(builder.AddSectionFromFile("/Main.as") < 0){
				SPRaise("Failed to load '/Main.as'.");
			}
			SPLog("Building");
			if(builder.BuildModule() < 0){
				SPRaise("Failed to build at least one of the scripts.");
			}
			
		}catch(...){
			engine->Release();
			throw;
		}
	}
	
	static std::string ASErrorToString(int ret){
		SPADES_MARK_FUNCTION();
		switch(ret){
			case asSUCCESS:
				return "Success";
			case asERROR:
				return "Unknown script engine error";
			case asCONTEXT_NOT_FINISHED:
				return "Context not finished";
			case asCONTEXT_NOT_PREPARED:
				return "Context not prepared";
			case asINVALID_ARG:
				return "Invalid argument";
			case asNO_FUNCTION:
				return "No function";
			case asNOT_SUPPORTED:
				return "Not supported";
			case asINVALID_NAME:
				return "Invalid name";
			case asNAME_TAKEN:
				return "Name taken";
			case asINVALID_DECLARATION:
				return "Invalid declaration";
			case asINVALID_OBJECT:
				return "Invalid object";
			case asINVALID_TYPE:
				return "Invalid type";
			case asALREADY_REGISTERED:
				return "Already registered";
			case asMULTIPLE_FUNCTIONS:
				return "Multiple functions";
			case asNO_MODULE:
				return "No module";
			case asNO_GLOBAL_VAR:
				return "No global variable";
			case asINVALID_CONFIGURATION:
				return "Invalid configuration";
			case asINVALID_INTERFACE:
				return "Invalid interface";
			case asCANT_BIND_ALL_FUNCTIONS:
				return "Failed to bind all functions";
			case asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
				return "A lower array dimension is not registered";
			case asWRONG_CONFIG_GROUP:
				return "Wrong config group";
			case asCONFIG_GROUP_IS_IN_USE:
				return "Config group is in use";
			case asILLEGAL_BEHAVIOUR_FOR_TYPE:
				return "Illegal behaviour for the given type";
			case asWRONG_CALLING_CONV:
				return "Wrong call convention";
			case asBUILD_IN_PROGRESS:
				return "Build in progress";
			case asINIT_GLOBAL_VARS_FAILED:
				return "Failed to initialize global variables";
			case asOUT_OF_MEMORY:
				return "Out of memory";
			default:
				return "Unknown error code";
		}
	}
	
	void ScriptManager::CheckError(int ret){
		if(ret < 0){
			SPADES_MARK_FUNCTION();
			SPRaise("%s", ASErrorToString(ret).c_str());
		}
	}
	
	ScriptManager::~ScriptManager() {
		SPADES_MARK_FUNCTION();
		engine->Release();
	}
	
	ScriptContextHandle ScriptManager::GetContext() {
		SPADES_MARK_FUNCTION_DEBUG();
		AutoLocker locker(&contextMutex);
		if(contextFreeList.empty()){
			// no free context; create one
			Context *ctx = new Context();
			ctx->obj = engine->CreateContext();
			ctx->refCount = 0;
			return ScriptContextHandle(ctx, this);
		}else{
			// get one
			Context *ctx = contextFreeList.front();
			contextFreeList.pop_front();
			SPAssert(ctx->refCount == 0);
			return ScriptContextHandle(ctx, this);
		}
	}
	
	ScriptContextHandle::ScriptContextHandle():
	manager(NULL), obj(NULL){
		
	}
	
	ScriptContextHandle::ScriptContextHandle(ScriptManager::Context *ctx,
											 ScriptManager *manager):
	manager(manager), obj(ctx){
		AutoLocker locker(&manager->contextMutex);
		ctx->refCount++;
	}
	
	ScriptContextHandle::ScriptContextHandle(const ScriptContextHandle& h) :
	manager(h.manager), obj(h.obj){
		AutoLocker locker(&manager->contextMutex);
		obj->refCount++;
	}
	
	ScriptContextHandle::~ScriptContextHandle() {
		Release();
	}
	
	void ScriptContextHandle::Release() {
		if(obj){
			AutoLocker locker(&manager->contextMutex);
			obj->refCount--;
			if(obj->refCount == 0){
				// this context is no longer used;
				// add to freelist
				manager->contextFreeList.push_back(obj);
			}
			
			obj = NULL;
			manager = NULL;
		}
	}
	
	void ScriptContextHandle::operator=(const spades::ScriptContextHandle & h) {
		if(h.obj == obj) return;
		Release();
		
		manager = h.manager;
		obj = h.obj;
		AutoLocker locker(&manager->contextMutex);
		obj->refCount++;
	}
	
	asIScriptContext *ScriptContextHandle::GetContext() const {
		SPADES_MARK_FUNCTION_DEBUG();
		SPAssert(obj != NULL);
		return obj->obj;
	}
	
	asIScriptContext *ScriptContextHandle::operator->() const {
		SPADES_MARK_FUNCTION_DEBUG();
		return GetContext();
	}
	
	void ScriptContextHandle::ExecuteChecked() {
		SPADES_MARK_FUNCTION();
		ScriptContextUtils(GetContext()).ExecuteChecked();
	}
	
	static std::map<std::string, ScriptObjectRegistrar *> * registrars = NULL;
	
	ScriptObjectRegistrar::ScriptObjectRegistrar(const std::string& name):
	name(name){
		if(!registrars)
			registrars = new std::map<std::string, ScriptObjectRegistrar *>();
		if(registrars->find(name) != registrars->end()){
			SPLog("WARNING: there are more than one ScriptObjectRegistrar with name '%s'", name.c_str());
			
		}
		(*registrars)[name]=this;
		std::fill(phaseDone, phaseDone + PhaseCount, false);
	}
	
	void ScriptObjectRegistrar::RegisterOne(const std::string &name,
											ScriptManager *manager,
											Phase phase) {
		std::map<std::string, ScriptObjectRegistrar *>::iterator it;
		it = registrars->find(name);
		if(it == registrars->end()){
			SPLog("WARNING: ScriptObjectRegistrar with name '%s' not found", name.c_str());
		}else{
			ScriptObjectRegistrar *r = it->second;
			if(r->phaseDone[(int)phase])
				return;
			r->phaseDone[(int)phase] = true;
			r->Register(manager,phase);
		}
	}
	
	void ScriptObjectRegistrar::RegisterAll(ScriptManager *manager, Phase phase) {
		std::map<std::string, ScriptObjectRegistrar *>::iterator it;
		for(it = registrars->begin(); it!= registrars->end(); it++){
			ScriptObjectRegistrar *r = it->second;
			if(r->phaseDone[(int)phase])
				continue;
			r->phaseDone[(int)phase] = true;
			r->Register(manager,phase);
		}
	}
	
#pragma mark - Context Utils
	
	ScriptContextUtils::ScriptContextUtils():
	context(asGetActiveContext()){
		
	}
	
	ScriptContextUtils::ScriptContextUtils(asIScriptContext *ctx):
	context(ctx){}
	
	void ScriptContextUtils::appendLocation( std::stringstream& ss, asIScriptFunction* func, const char *secName, int line, int column )
	{
		ss << "[" << (secName?secName:"(stub)") << ":" << line << "," << column << "] " << func->GetDeclaration(true, true);
	}

	void ScriptContextUtils::ExecuteChecked() {
		SPADES_MARK_FUNCTION();
		int r = context->Execute();
		ScriptManager::CheckError(r);
		if(r == asEXECUTION_ABORTED) {
			SPRaise("Script execution aborted.");
		}else if(r == asEXECUTION_SUSPENDED) {
			SPRaise("Script execution suspended."); // TODO: shouldn't raise error?
		}else if(r == asEXECUTION_EXCEPTION) {
			const char *secName = NULL;
			int line = 0, column = 0;
			asIScriptFunction *func = context->GetExceptionFunction();
			line = context->GetExceptionLineNumber(&column, &secName);
			std::stringstream ss;
			ss << context->GetExceptionString() << " @ ";
			appendLocation( ss, func, secName, line, column );
			asUINT num = context->GetCallstackSize();
			for( asUINT n = 1; n < num; ++n ) {		//skip entry 0, that's the current / exception addr
				func = context->GetFunction( n );
				line = context->GetLineNumber( n, &column, &secName);
				ss << std::endl << " > ";
				appendLocation( ss, func, secName, line, column );
			}
			std::string tmp = ss.str();
			SPRaise( tmp.c_str() );
		}
	}
	
	void ScriptContextUtils::SetNativeException(const std::exception &ex) {
		context->SetException(ex.what());
	}
	
	
#pragma mark - Some Script Functions
	
	class SomeScriptFunctionRegistrar: public ScriptObjectRegistrar {
	public:
		SomeScriptFunctionRegistrar():
		ScriptObjectRegistrar("SomeScriptFunc"){}
		
		static void Raise(const std::string& str) {
			asGetActiveContext()->SetException(str.c_str());
		}
		
		static void Assert(bool cond) {
			if(!cond){
				Raise("Assertion failed");
			}
		}
	
		static void NotImplemented() {
			Raise("Not implemented");
		}
		
		virtual void Register(ScriptManager *manager, Phase phase){
			asIScriptEngine *eng = manager->GetEngine();
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseGlobalFunction:
					eng->RegisterGlobalFunction("void Raise(const string& in)", asFUNCTION(Raise),
												asCALL_CDECL);
					eng->RegisterGlobalFunction("void Assert(bool cond)", asFUNCTION(Assert),
												asCALL_CDECL);
					eng->RegisterGlobalFunction("void NotImplemented()", asFUNCTION(NotImplemented),
												asCALL_CDECL);
					break;
				default:
					break;
			}
		}
		
	};
	
	static SomeScriptFunctionRegistrar ssfReg;
}
