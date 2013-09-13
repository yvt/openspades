#ifndef SCRIPTWEAKREF_H
#define SCRIPTWEAKREF_H

// The CScriptWeakRef class was originally implemented by vroad in March 2013

#ifndef ANGELSCRIPT_H 
// Avoid having to inform include path if header is already include before
#include "angelscript.h"
#endif


BEGIN_AS_NAMESPACE

class CScriptWeakRef 
{
public:
	// Constructors
	CScriptWeakRef(asIObjectType *type);
	CScriptWeakRef(const CScriptWeakRef &other);
	CScriptWeakRef(void *ref, asIObjectType *type);

	// Memory management
	void AddRef() const;
	void Release() const;

	// Copy the stored value from another weakref object
	CScriptWeakRef &operator=(const CScriptWeakRef &other);

	// Compare equalness
	bool operator==(const CScriptWeakRef &o) const;
	bool operator!=(const CScriptWeakRef &o) const;

	// Returns the object if it is still alive
	void *Get() const;

	// Returns the type of the reference held
	asIObjectType *GetRefType() const;

protected:
	~CScriptWeakRef();

	// These functions need to have access to protected
	// members in order to call them from the script engine
	friend void RegisterScriptWeakRef_Native(asIScriptEngine *engine);

	mutable int            refCount;
	void                  *m_ref;
	asIObjectType         *m_type;
	asILockableSharedBool *m_weakRefFlag;
};

void RegisterScriptWeakRef(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif