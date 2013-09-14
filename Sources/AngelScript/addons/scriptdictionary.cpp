#include <assert.h>
#include <string.h>
#include "scriptdictionary.h"
#include "scriptarray.h"

BEGIN_AS_NAMESPACE

using namespace std;

//--------------------------------------------------------------------------
// CScriptDictionary implementation

CScriptDictionary::CScriptDictionary(asIScriptEngine *engine)
{
    // We start with one reference
    refCount = 1;
	gcFlag = false;

    // Keep a reference to the engine for as long as we live
	// We don't increment the reference counter, because the 
	// engine will hold a pointer to the object. 
    this->engine = engine;

	// Notify the garbage collector of this object
	// TODO: The type id should be cached
	engine->NotifyGarbageCollectorOfNewObject(this, engine->GetObjectTypeByName("dictionary"));
}

CScriptDictionary::~CScriptDictionary()
{
    // Delete all keys and values
    DeleteAll();
}

void CScriptDictionary::AddRef() const
{
	// We need to clear the GC flag
	gcFlag = false;
	asAtomicInc(refCount);
}

void CScriptDictionary::Release() const
{
	// We need to clear the GC flag
	gcFlag = false;
	if( asAtomicDec(refCount) == 0 )
        delete this;
}

int CScriptDictionary::GetRefCount()
{
	return refCount;
}

void CScriptDictionary::SetGCFlag()
{
	gcFlag = true;
}

bool CScriptDictionary::GetGCFlag()
{
	return gcFlag;
}

void CScriptDictionary::EnumReferences(asIScriptEngine *engine)
{
	// Call the gc enum callback for each of the objects
    map<string, valueStruct>::iterator it;
    for( it = dict.begin(); it != dict.end(); it++ )
    {
		if( it->second.typeId & asTYPEID_MASK_OBJECT )
			engine->GCEnumCallback(it->second.valueObj);
    }
}

void CScriptDictionary::ReleaseAllReferences(asIScriptEngine * /*engine*/)
{
	// We're being told to release all references in 
	// order to break circular references for dead objects
	DeleteAll();
}

CScriptDictionary &CScriptDictionary::operator =(const CScriptDictionary &other)
{
	// Clear everything we had before
	DeleteAll();

	// Do a shallow copy of the dictionary
    map<string, valueStruct>::const_iterator it;
    for( it = other.dict.begin(); it != other.dict.end(); it++ )
    {
		if( it->second.typeId & asTYPEID_OBJHANDLE )
			Set(it->first, (void*)&it->second.valueObj, it->second.typeId);
		else if( it->second.typeId & asTYPEID_MASK_OBJECT )
			Set(it->first, (void*)it->second.valueObj, it->second.typeId);
		else
			Set(it->first, (void*)&it->second.valueInt, it->second.typeId);
    }

    return *this;
}

void CScriptDictionary::Set(const string &key, void *value, int typeId)
{
	valueStruct valStruct = {{0},0};
	valStruct.typeId = typeId;
	if( typeId & asTYPEID_OBJHANDLE )
	{
		// We're receiving a reference to the handle, so we need to dereference it
		valStruct.valueObj = *(void**)value;
		engine->AddRefScriptObject(valStruct.valueObj, engine->GetObjectTypeById(typeId));
	}
	else if( typeId & asTYPEID_MASK_OBJECT )
	{
		// Create a copy of the object
		valStruct.valueObj = engine->CreateScriptObjectCopy(value, engine->GetObjectTypeById(typeId));
	}
	else
	{
		// Copy the primitive value
		// We receive a pointer to the value.
		int size = engine->GetSizeOfPrimitiveType(typeId);
		memcpy(&valStruct.valueInt, value, size);
	}

    map<string, valueStruct>::iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        FreeValue(it->second);

        // Insert the new value
        it->second = valStruct;
    }
    else
    {
        dict.insert(map<string, valueStruct>::value_type(key, valStruct));
    }
}

// This overloaded method is implemented so that all integer and
// unsigned integers types will be stored in the dictionary as int64
// through implicit conversions. This simplifies the management of the
// numeric types when the script retrieves the stored value using a 
// different type.
void CScriptDictionary::Set(const string &key, asINT64 &value)
{
	Set(key, &value, asTYPEID_INT64);
}

// This overloaded method is implemented so that all floating point types 
// will be stored in the dictionary as double through implicit conversions. 
// This simplifies the management of the numeric types when the script 
// retrieves the stored value using a different type.
void CScriptDictionary::Set(const string &key, double &value)
{
	Set(key, &value, asTYPEID_DOUBLE);
}

// Returns true if the value was successfully retrieved
bool CScriptDictionary::Get(const string &key, void *value, int typeId) const
{
    map<string, valueStruct>::const_iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        // Return the value
		if( typeId & asTYPEID_OBJHANDLE )
		{
			// A handle can be retrieved if the stored type is a handle of same or compatible type
			// or if the stored type is an object that implements the interface that the handle refer to.
			if( (it->second.typeId & asTYPEID_MASK_OBJECT) && 
				engine->IsHandleCompatibleWithObject(it->second.valueObj, it->second.typeId, typeId) )
			{
				engine->AddRefScriptObject(it->second.valueObj, engine->GetObjectTypeById(it->second.typeId));
				*(void**)value = it->second.valueObj;

				return true;
			}
		}
		else if( typeId & asTYPEID_MASK_OBJECT )
		{
			// Verify that the copy can be made
			bool isCompatible = false;
			if( it->second.typeId == typeId )
				isCompatible = true;

			// Copy the object into the given reference
			if( isCompatible )
			{
				engine->AssignScriptObject(value, it->second.valueObj, engine->GetObjectTypeById(typeId));

				return true;
			}
		}
		else
		{
			if( it->second.typeId == typeId )
			{
				int size = engine->GetSizeOfPrimitiveType(typeId);
				memcpy(value, &it->second.valueInt, size);
				return true;
			}

			// We know all numbers are stored as either int64 or double, since we register overloaded functions for those
			if( it->second.typeId == asTYPEID_INT64 && typeId == asTYPEID_DOUBLE )
			{
				*(double*)value = double(it->second.valueInt);
				return true;
			}
			else if( it->second.typeId == asTYPEID_DOUBLE && typeId == asTYPEID_INT64 )
			{
				*(asINT64*)value = asINT64(it->second.valueFlt);
				return true;
			}
		}
    }

    // AngelScript has already initialized the value with a default value,
    // so we don't have to do anything if we don't find the element, or if 
	// the element is incompatible with the requested type.

	return false;
}

bool CScriptDictionary::Get(const string &key, asINT64 &value) const
{
	return Get(key, &value, asTYPEID_INT64);
}

bool CScriptDictionary::Get(const string &key, double &value) const
{
	return Get(key, &value, asTYPEID_DOUBLE);
}

bool CScriptDictionary::Exists(const string &key) const
{
    map<string, valueStruct>::const_iterator it;
    it = dict.find(key);
    if( it != dict.end() )
        return true;

    return false;
}

bool CScriptDictionary::IsEmpty() const
{
	if( dict.size() == 0 )
		return true;

	return false;
}

asUINT CScriptDictionary::GetSize() const
{
	return asUINT(dict.size());
}

void CScriptDictionary::Delete(const string &key)
{
    map<string, valueStruct>::iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        FreeValue(it->second);
        dict.erase(it);
    }
}

void CScriptDictionary::DeleteAll()
{
    map<string, valueStruct>::iterator it;
    for( it = dict.begin(); it != dict.end(); it++ )
        FreeValue(it->second);

    dict.clear();
}

void CScriptDictionary::FreeValue(valueStruct &value)
{
    // If it is a handle or a ref counted object, call release
	if( value.typeId & asTYPEID_MASK_OBJECT )
	{
		// Let the engine release the object
		engine->ReleaseScriptObject(value.valueObj, engine->GetObjectTypeById(value.typeId));
		value.valueObj = 0;
		value.typeId = 0;
	}

    // For primitives, there's nothing to do
}

CScriptArray* CScriptDictionary::GetKeys() const
{
	// TODO: optimize: The string array type should only be determined once. 
	//                 It should be recomputed when registering the dictionary class.
	//                 Only problem is if multiple engines are used, as they may not
	//                 share the same type id. Alternatively it can be stored in the 
	//                 user data for the dictionary type.
	int stringArrayType = engine->GetTypeIdByDecl("array<string>");
	asIObjectType *ot = engine->GetObjectTypeById(stringArrayType);

	// Create the array object
	CScriptArray *array = new CScriptArray(asUINT(dict.size()), ot);
	long current = -1;
	std::map<string, valueStruct>::const_iterator it;
	for( it = dict.begin(); it != dict.end(); it++ )
	{
		current++;
		*(string*)array->At(current) = it->first;
	}

	return array;
}

//--------------------------------------------------------------------------
// Generic wrappers

void ScriptDictionaryFactory_Generic(asIScriptGeneric *gen)
{
    *(CScriptDictionary**)gen->GetAddressOfReturnLocation() = new CScriptDictionary(gen->GetEngine());
}

void ScriptDictionaryAddRef_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    dict->AddRef();
}

void ScriptDictionaryRelease_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    dict->Release();
}

void ScriptDictionaryAssign_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    CScriptDictionary *other = *(CScriptDictionary**)gen->GetAddressOfArg(0);
	*dict = *other;
	*(CScriptDictionary**)gen->GetAddressOfReturnLocation() = dict;
}

void ScriptDictionarySet_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    void *ref = *(void**)gen->GetAddressOfArg(1);
    int typeId = gen->GetArgTypeId(1);
    dict->Set(*key, ref, typeId);
}

void ScriptDictionarySetInt_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    void *ref = *(void**)gen->GetAddressOfArg(1);
    dict->Set(*key, *(asINT64*)ref);
}

void ScriptDictionarySetFlt_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    void *ref = *(void**)gen->GetAddressOfArg(1);
    dict->Set(*key, *(double*)ref);
}

void ScriptDictionaryGet_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    void *ref = *(void**)gen->GetAddressOfArg(1);
    int typeId = gen->GetArgTypeId(1);
    *(bool*)gen->GetAddressOfReturnLocation() = dict->Get(*key, ref, typeId);
}

void ScriptDictionaryGetInt_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    void *ref = *(void**)gen->GetAddressOfArg(1);
    *(bool*)gen->GetAddressOfReturnLocation() = dict->Get(*key, *(asINT64*)ref);
}

void ScriptDictionaryGetFlt_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    void *ref = *(void**)gen->GetAddressOfArg(1);
    *(bool*)gen->GetAddressOfReturnLocation() = dict->Get(*key, *(double*)ref);
}

void ScriptDictionaryExists_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    bool ret = dict->Exists(*key);
    *(bool*)gen->GetAddressOfReturnLocation() = ret;
}

void ScriptDictionaryDelete_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetAddressOfArg(0);
    dict->Delete(*key);
}

void ScriptDictionaryDeleteAll_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    dict->DeleteAll();
}

static void ScriptDictionaryGetRefCount_Generic(asIScriptGeneric *gen)
{
	CScriptDictionary *self = (CScriptDictionary*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = self->GetRefCount();
}

static void ScriptDictionarySetGCFlag_Generic(asIScriptGeneric *gen)
{
	CScriptDictionary *self = (CScriptDictionary*)gen->GetObject();
	self->SetGCFlag();
}

static void ScriptDictionaryGetGCFlag_Generic(asIScriptGeneric *gen)
{
	CScriptDictionary *self = (CScriptDictionary*)gen->GetObject();
	*(bool*)gen->GetAddressOfReturnLocation() = self->GetGCFlag();
}

static void ScriptDictionaryEnumReferences_Generic(asIScriptGeneric *gen)
{
	CScriptDictionary *self = (CScriptDictionary*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->EnumReferences(engine);
}

static void ScriptDictionaryReleaseAllReferences_Generic(asIScriptGeneric *gen)
{
	CScriptDictionary *self = (CScriptDictionary*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->ReleaseAllReferences(engine);
}

static void CScriptDictionaryGetKeys_Generic(asIScriptGeneric *gen)
{
	CScriptDictionary *self = (CScriptDictionary*)gen->GetObject();
	*(CScriptArray**)gen->GetAddressOfReturnLocation() = self->GetKeys();
}

//--------------------------------------------------------------------------
// Register the type

void RegisterScriptDictionary(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptDictionary_Generic(engine);
	else
		RegisterScriptDictionary_Native(engine);
}

void RegisterScriptDictionary_Native(asIScriptEngine *engine)
{
	int r;

    r = engine->RegisterObjectType("dictionary", sizeof(CScriptDictionary), asOBJ_REF | asOBJ_GC); assert( r >= 0 );
	// Use the generic interface to construct the object since we need the engine pointer, we could also have retrieved the engine pointer from the active context
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_FACTORY, "dictionary@ f()", asFUNCTION(ScriptDictionaryFactory_Generic), asCALL_GENERIC); assert( r>= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptDictionary,AddRef), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptDictionary,Release), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("dictionary", "dictionary &opAssign(const dictionary &in)", asMETHODPR(CScriptDictionary, operator=, (const CScriptDictionary &), CScriptDictionary&), asCALL_THISCALL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "void set(const string &in, ?&in)", asMETHODPR(CScriptDictionary,Set,(const string&,void*,int),void), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool get(const string &in, ?&out) const", asMETHODPR(CScriptDictionary,Get,(const string&,void*,int) const,bool), asCALL_THISCALL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "void set(const string &in, int64&in)", asMETHODPR(CScriptDictionary,Set,(const string&,asINT64&),void), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool get(const string &in, int64&out) const", asMETHODPR(CScriptDictionary,Get,(const string&,asINT64&) const,bool), asCALL_THISCALL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "void set(const string &in, double&in)", asMETHODPR(CScriptDictionary,Set,(const string&,double&),void), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool get(const string &in, double&out) const", asMETHODPR(CScriptDictionary,Get,(const string&,double&) const,bool), asCALL_THISCALL); assert( r >= 0 );
    
	r = engine->RegisterObjectMethod("dictionary", "bool exists(const string &in) const", asMETHOD(CScriptDictionary,Exists), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("dictionary", "bool isEmpty() const", asMETHOD(CScriptDictionary, IsEmpty), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("dictionary", "uint getSize() const", asMETHOD(CScriptDictionary, GetSize), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void delete(const string &in)", asMETHOD(CScriptDictionary,Delete), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void deleteAll()", asMETHOD(CScriptDictionary,DeleteAll), asCALL_THISCALL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "array<string> @getKeys() const", asMETHOD(CScriptDictionary,GetKeys), asCALL_THISCALL); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(CScriptDictionary,GetRefCount), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(CScriptDictionary,SetGCFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(CScriptDictionary,GetGCFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(CScriptDictionary,EnumReferences), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(CScriptDictionary,ReleaseAllReferences), asCALL_THISCALL); assert( r >= 0 );

#if AS_USE_STLNAMES == 1
	// Same as isEmpty
	r = engine->RegisterObjectMethod("dictionary", "bool empty() const", asMETHOD(CScriptDictionary, IsEmpty), asCALL_THISCALL); assert( r >= 0 );
	// Same as getSize
	r = engine->RegisterObjectMethod("dictionary", "uint size() const", asMETHOD(CScriptDictionary, GetSize), asCALL_THISCALL); assert( r >= 0 );
	// Same as delete
    r = engine->RegisterObjectMethod("dictionary", "void erase(const string &in)", asMETHOD(CScriptDictionary,Delete), asCALL_THISCALL); assert( r >= 0 );
	// Same as deleteAll
	r = engine->RegisterObjectMethod("dictionary", "void clear()", asMETHOD(CScriptDictionary,DeleteAll), asCALL_THISCALL); assert( r >= 0 );
#endif
}

void RegisterScriptDictionary_Generic(asIScriptEngine *engine)
{
    int r;

    r = engine->RegisterObjectType("dictionary", sizeof(CScriptDictionary), asOBJ_REF | asOBJ_GC); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_FACTORY, "dictionary@ f()", asFUNCTION(ScriptDictionaryFactory_Generic), asCALL_GENERIC); assert( r>= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptDictionaryAddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptDictionaryRelease_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectMethod("dictionary", "dictionary &opAssign(const dictionary &in)", asFUNCTION(ScriptDictionaryAssign_Generic), asCALL_GENERIC); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "void set(const string &in, ?&in)", asFUNCTION(ScriptDictionarySet_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool get(const string &in, ?&out) const", asFUNCTION(ScriptDictionaryGet_Generic), asCALL_GENERIC); assert( r >= 0 );
    
    r = engine->RegisterObjectMethod("dictionary", "void set(const string &in, int64&in)", asFUNCTION(ScriptDictionarySetInt_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool get(const string &in, int64&out) const", asFUNCTION(ScriptDictionaryGetInt_Generic), asCALL_GENERIC); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "void set(const string &in, double&in)", asFUNCTION(ScriptDictionarySetFlt_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool get(const string &in, double&out) const", asFUNCTION(ScriptDictionaryGetFlt_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectMethod("dictionary", "bool exists(const string &in) const", asFUNCTION(ScriptDictionaryExists_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void delete(const string &in)", asFUNCTION(ScriptDictionaryDelete_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void deleteAll()", asFUNCTION(ScriptDictionaryDeleteAll_Generic), asCALL_GENERIC); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "array<string> @getKeys() const", asFUNCTION(CScriptDictionaryGetKeys_Generic), asCALL_GENERIC); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptDictionaryGetRefCount_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptDictionarySetGCFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptDictionaryGetGCFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptDictionaryEnumReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptDictionaryReleaseAllReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
}

END_AS_NAMESPACE


