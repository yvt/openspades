
// The CScriptWeakRef class was originally implemented by vroad in March 2013

#include "weakref.h"
#include <new>
#include <assert.h>
#include <string.h> // strstr()

BEGIN_AS_NAMESPACE

static CScriptWeakRef* ScriptWeakRefFactory(asIObjectType *type)
{
	return new CScriptWeakRef(type);
}

static CScriptWeakRef* ScriptWeakRefFactory2(asIObjectType *type, void *ref)
{
	CScriptWeakRef *wr = new CScriptWeakRef(ref, type);

	// It's possible the constructor raised a script exception, in which case we 
	// need to free the memory and return null instead, else we get a memory leak.
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && ctx->GetState() == asEXECUTION_EXCEPTION )
	{
		wr->Release();
		return 0;
	}

	return wr;
}

static bool ScriptWeakRefTemplateCallback(asIObjectType *ot, bool &/*dontGarbageCollect*/)
{
	asIObjectType *subType = ot->GetSubType();

	// Weak references only work for reference types
	if( subType == 0 ) return false;
	if( !(subType->GetFlags() & asOBJ_REF) ) return false;
	
	// The subtype shouldn't be a handle
	if( ot->GetSubTypeId() & asTYPEID_OBJHANDLE )
		return false;

	// Make sure the type really supports weak references
	asUINT cnt = subType->GetBehaviourCount();
	for( asUINT n = 0; n < cnt; n++ )
	{
		asEBehaviours beh;
		subType->GetBehaviourByIndex(n, &beh);
		if( beh == asBEHAVE_GET_WEAKREF_FLAG )
			return true;
	}

	return false;
}

CScriptWeakRef::CScriptWeakRef(asIObjectType *type)
{
	refCount = 1;
	m_ref  = 0;
	m_type = type;
	m_type->AddRef();
	m_weakRefFlag = 0;
}

CScriptWeakRef::CScriptWeakRef(const CScriptWeakRef &other)
{
	refCount = 1;
	m_ref  = other.m_ref;
	m_type = other.m_type;
	m_type->AddRef();
	m_weakRefFlag = other.m_weakRefFlag;
	if( m_weakRefFlag )
		m_weakRefFlag->AddRef();
}

CScriptWeakRef::CScriptWeakRef(void *ref, asIObjectType *type)
{
	refCount = 1;
	m_ref  = ref;
	m_type = type;
	m_type->AddRef();

	// Get the shared flag that will tell us when the object has been destroyed
	// This is threadsafe as we hold a strong reference to the object 
	m_weakRefFlag = m_type->GetEngine()->GetWeakRefFlagOfScriptObject(m_ref, m_type->GetSubType());
	if( m_weakRefFlag )
		m_weakRefFlag->AddRef();

	// Release the handle that was received, since the weakref isn't suppose to prevent the object from being destroyed
	m_type->GetEngine()->ReleaseScriptObject(m_ref, m_type->GetSubType());
}

CScriptWeakRef::~CScriptWeakRef()
{
	if( m_type )
		m_type->Release();
	if( m_weakRefFlag )
		m_weakRefFlag->Release();
}

void CScriptWeakRef::AddRef() const
{
	asAtomicInc(refCount);
}

void CScriptWeakRef::Release() const
{
	if( asAtomicDec(refCount) == 0 )
	{
		// When reaching 0 no more references to this instance 
		// exists and the object should be destroyed
		delete this;
	}
}

CScriptWeakRef &CScriptWeakRef::operator =(const CScriptWeakRef &other)
{
	// Don't do anything if it is the same reference
	if( m_ref == other.m_ref )
		return *this;

	m_ref  = other.m_ref;

	if( m_type )
		m_type->Release();
	m_type = other.m_type;
	if( m_type )
		m_type->AddRef();

	if( m_weakRefFlag )
		m_weakRefFlag->Release();
	m_weakRefFlag = other.m_weakRefFlag;
	if( m_weakRefFlag )
		m_weakRefFlag->AddRef();

	return *this;
}

asIObjectType *CScriptWeakRef::GetRefType() const
{
	return m_type->GetSubType();
}

bool CScriptWeakRef::operator==(const CScriptWeakRef &o) const
{
	if( m_ref  == o.m_ref &&
		m_type == o.m_type )
		return true;

	// TODO: If type is not the same, we should attempt to do a dynamic cast,
	//       which may change the pointer for application registered classes

	return false;
}

bool CScriptWeakRef::operator!=(const CScriptWeakRef &o) const
{
	return !(*this == o);
}

// AngelScript: used as '@obj = ref.get();'
void *CScriptWeakRef::Get() const
{
	// If we hold a null handle, then just return null
	if( m_ref == 0 || m_weakRefFlag == 0 )
		return 0;
	
	// Lock on the shared bool, so we can be certain it won't be changed to true
	// between the inspection of the flag and the increase of the ref count in the
	// owning object.
	m_weakRefFlag->Lock();
	if( !m_weakRefFlag->Get() )
	{
		m_type->GetEngine()->AddRefScriptObject(m_ref, m_type->GetSubType());
		m_weakRefFlag->Unlock();
		return m_ref;
	}
	m_weakRefFlag->Unlock();

	return 0;
}

void RegisterScriptWeakRef_Native(asIScriptEngine *engine)
{
	int r;

	// Register a type for non-const handles
	r = engine->RegisterObjectType("weakref<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_FACTORY, "weakref<T>@ f(int&in)", asFUNCTION(ScriptWeakRefFactory), asCALL_CDECL); assert( r>= 0 );
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_FACTORY, "weakref<T>@ f(int&in, T@)", asFUNCTION(ScriptWeakRefFactory2), asCALL_CDECL); assert( r>= 0 );
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptWeakRefTemplateCallback), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptWeakRef,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptWeakRef,Release), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("weakref<T>", "T@ get() const", asMETHODPR(CScriptWeakRef, Get, () const, void*), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("weakref<T>", "weakref<T> &opAssign(const weakref<T> &in)", asMETHOD(CScriptWeakRef, operator=), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("weakref<T>", "bool opEquals(const weakref<T> &in) const", asMETHODPR(CScriptWeakRef, operator==, (const CScriptWeakRef &) const, bool), asCALL_THISCALL); assert( r >= 0 );

	// Register another type for const handles
	r = engine->RegisterObjectType("const_weakref<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_FACTORY, "const_weakref<T>@ f(int&in)", asFUNCTION(ScriptWeakRefFactory), asCALL_CDECL); assert( r>= 0 );
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_FACTORY, "const_weakref<T>@ f(int&in, const T@)", asFUNCTION(ScriptWeakRefFactory2), asCALL_CDECL); assert( r>= 0 );
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptWeakRefTemplateCallback), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptWeakRef,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptWeakRef,Release), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("const_weakref<T>", "const T@ get() const", asMETHODPR(CScriptWeakRef, Get, () const, void*), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opAssign(const const_weakref<T> &in)", asMETHOD(CScriptWeakRef, operator=), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const const_weakref<T> &in) const", asMETHODPR(CScriptWeakRef, operator==, (const CScriptWeakRef &) const, bool), asCALL_THISCALL); assert( r >= 0 );

	// Allow non-const weak references to be converted to const weak references
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opAssign(const weakref<T> &in)", asMETHOD(CScriptWeakRef, operator=), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const weakref<T> &in) const", asMETHODPR(CScriptWeakRef, operator==, (const CScriptWeakRef &) const, bool), asCALL_THISCALL); assert( r >= 0 );
}

static void ScriptWeakRefAddRef_Generic(asIScriptGeneric *gen)
{
	CScriptWeakRef *self = (CScriptWeakRef*)gen->GetObject();
	self->AddRef();
}

static void ScriptWeakRefRelease_Generic(asIScriptGeneric *gen)
{
	CScriptWeakRef *self = (CScriptWeakRef*)gen->GetObject();
	self->Release();
}

void CScriptWeakRef_Get_Generic(asIScriptGeneric *gen)
{
	int typeId = gen->GetArgTypeId(0);
	CScriptWeakRef *self = reinterpret_cast<CScriptWeakRef*>(gen->GetObject());
	gen->SetReturnAddress(self->Get());
}

void CScriptWeakRef_Assign_Generic(asIScriptGeneric *gen)
{
	CScriptWeakRef *other = reinterpret_cast<CScriptWeakRef*>(gen->GetArgAddress(0));
	CScriptWeakRef *self = reinterpret_cast<CScriptWeakRef*>(gen->GetObject());
	*self = *other;
	gen->SetReturnAddress(self);
}

void CScriptWeakRef_Equals_Generic(asIScriptGeneric *gen)
{
	CScriptWeakRef *other = reinterpret_cast<CScriptWeakRef*>(gen->GetArgAddress(0));
	CScriptWeakRef *self = reinterpret_cast<CScriptWeakRef*>(gen->GetObject());
	gen->SetReturnByte(*self == *other);
}

void RegisterScriptWeakRef_Generic(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterObjectType("weakref<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptWeakRefTemplateCallback), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptWeakRefAddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptWeakRefRelease_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectMethod("weakref<T>", "T get()", asFUNCTION(CScriptWeakRef_Get_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("weakref<T>", "ref &opAssign(const ref &in)", asFUNCTION(CScriptWeakRef_Assign_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("weakref<T>", "bool opEquals(const ref &in) const", asFUNCTION(CScriptWeakRef_Equals_Generic), asCALL_GENERIC); assert( r >= 0 );
}

void RegisterScriptWeakRef(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptWeakRef_Generic(engine);
	else
		RegisterScriptWeakRef_Native(engine);
}


END_AS_NAMESPACE