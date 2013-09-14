#include <string.h>
#include "scripthelper.h"
#include <string>
#include <assert.h>
#include <stdio.h>

using namespace std;

BEGIN_AS_NAMESPACE

int CompareRelation(asIScriptEngine *engine, void *lobj, void *robj, int typeId, int &result)
{
    // TODO: If a lot of script objects are going to be compared, e.g. when sorting an array, 
    //       then the method id and context should be cached between calls.
    
	int retval = -1;
	asIScriptFunction *func = 0;

	asIObjectType *ot = engine->GetObjectTypeById(typeId);
	if( ot )
	{
		// Check if the object type has a compatible opCmp method
		for( asUINT n = 0; n < ot->GetMethodCount(); n++ )
		{
			asIScriptFunction *f = ot->GetMethodByIndex(n);
			asDWORD flags;
			if( strcmp(f->GetName(), "opCmp") == 0 &&
				f->GetReturnTypeId(&flags) == asTYPEID_INT32 &&
				flags == asTM_NONE &&
				f->GetParamCount() == 1 )
			{
				int paramTypeId = f->GetParamTypeId(0, &flags);
				
				// The parameter must be an input reference of the same type
				// If the reference is a inout reference, then it must also be read-only
				if( !(flags & asTM_INREF) || typeId != paramTypeId || ((flags & asTM_OUTREF) && !(flags & asTM_CONST)) )
					break;

				// Found the method
				func = f;
				break;
			}
		}
	}

	if( func )
	{
		// Call the method
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(func);
		ctx->SetObject(lobj);
		ctx->SetArgAddress(0, robj);
		int r = ctx->Execute();
		if( r == asEXECUTION_FINISHED )
		{
			result = (int)ctx->GetReturnDWord();

			// The comparison was successful
			retval = 0;
		}
		ctx->Release();
	}

	return retval;
}

int CompareEquality(asIScriptEngine *engine, void *lobj, void *robj, int typeId, bool &result)
{
    // TODO: If a lot of script objects are going to be compared, e.g. when searching for an
	//       entry in a set, then the method and context should be cached between calls.
    
	int retval = -1;
	asIScriptFunction *func = 0;

	asIObjectType *ot = engine->GetObjectTypeById(typeId);
	if( ot )
	{
		// Check if the object type has a compatible opEquals method
		for( asUINT n = 0; n < ot->GetMethodCount(); n++ )
		{
			asIScriptFunction *f = ot->GetMethodByIndex(n);
			asDWORD flags;
			if( strcmp(f->GetName(), "opEquals") == 0 &&
				f->GetReturnTypeId(&flags) == asTYPEID_BOOL &&
				flags == asTM_NONE &&
				f->GetParamCount() == 1 )
			{
				int paramTypeId = f->GetParamTypeId(0, &flags);
				
				// The parameter must be an input reference of the same type
				// If the reference is a inout reference, then it must also be read-only
				if( !(flags & asTM_INREF) || typeId != paramTypeId || ((flags & asTM_OUTREF) && !(flags & asTM_CONST)) )
					break;

				// Found the method
				func = f;
				break;
			}
		}
	}

	if( func )
	{
		// Call the method
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(func);
		ctx->SetObject(lobj);
		ctx->SetArgAddress(0, robj);
		int r = ctx->Execute();
		if( r == asEXECUTION_FINISHED )
		{
			result = ctx->GetReturnByte() ? true : false;

			// The comparison was successful
			retval = 0;
		}
		ctx->Release();
	}
	else
	{
		// If the opEquals method doesn't exist, then we try with opCmp instead
		int relation;
		retval = CompareRelation(engine, lobj, robj, typeId, relation);
		if( retval >= 0 )
			result = relation == 0 ? true : false;
	}

	return retval;
}

int ExecuteString(asIScriptEngine *engine, const char *code, asIScriptModule *mod, asIScriptContext *ctx)
{
	return ExecuteString(engine, code, 0, asTYPEID_VOID, mod, ctx);
}

int ExecuteString(asIScriptEngine *engine, const char *code, void *ref, int refTypeId, asIScriptModule *mod, asIScriptContext *ctx)
{
	// Wrap the code in a function so that it can be compiled and executed
	string funcCode = " ExecuteString() {\n";
	funcCode += code;
	funcCode += "\n;}";
	
	// Determine the return type based on the type of the ref arg
	funcCode = engine->GetTypeDeclaration(refTypeId, true) + funcCode;

	// GetModule will free unused types, so to be on the safe side we'll hold on to a reference to the type
	asIObjectType *type = 0;
	if( refTypeId & asTYPEID_MASK_OBJECT )
	{
		type = engine->GetObjectTypeById(refTypeId);
		if( type )
			type->AddRef();
	}
	
	// If no module was provided, get a dummy from the engine
	asIScriptModule *execMod = mod ? mod : engine->GetModule("ExecuteString", asGM_ALWAYS_CREATE);

	// Now it's ok to release the type
	if( type )
		type->Release();

	// Compile the function that can be executed
	asIScriptFunction *func = 0;
	int r = execMod->CompileFunction("ExecuteString", funcCode.c_str(), -1, 0, &func);
	if( r < 0 )
		return r;

	// If no context was provided, request a new one from the engine
	asIScriptContext *execCtx = ctx ? ctx : engine->CreateContext();
	r = execCtx->Prepare(func);
	if( r < 0 )
	{
		func->Release();
		if( !ctx ) execCtx->Release();
		return r;
	}

	// Execute the function
	r = execCtx->Execute();

	// Unless the provided type was void retrieve it's value
	if( ref != 0 && refTypeId != asTYPEID_VOID )
	{
		if( refTypeId & asTYPEID_OBJHANDLE )
		{
			// Expect the pointer to be null to start with
			assert( *reinterpret_cast<void**>(ref) == 0 );
			*reinterpret_cast<void**>(ref) = *reinterpret_cast<void**>(execCtx->GetAddressOfReturnValue());
			engine->AddRefScriptObject(*reinterpret_cast<void**>(ref), engine->GetObjectTypeById(refTypeId));
		}
		else if( refTypeId & asTYPEID_MASK_OBJECT )
		{
			// Expect the pointer to point to a valid object
			assert( *reinterpret_cast<void**>(ref) != 0 );
			engine->AssignScriptObject(ref, execCtx->GetAddressOfReturnValue(), engine->GetObjectTypeById(refTypeId));
		}
		else
		{
			// Copy the primitive value
			memcpy(ref, execCtx->GetAddressOfReturnValue(), engine->GetSizeOfPrimitiveType(refTypeId));
		}
	}
	
	// Clean up
	func->Release();
	if( !ctx ) execCtx->Release();

	return r;
}

int WriteConfigToFile(asIScriptEngine *engine, const char *filename)
{
	int c, n;

	FILE *f = 0;
#if _MSC_VER >= 1400 && !defined(__S3E__) 
	// MSVC 8.0 / 2005 introduced new functions 
	// Marmalade doesn't use these, even though it uses the MSVC compiler
	fopen_s(&f, filename, "wt");
#else
	f = fopen(filename, "wt");
#endif
	if( f == 0 )
		return -1;

	asDWORD currAccessMask = 0;
	string currNamespace = "";

	// Make sure the default array type is expanded to the template form 
	bool expandDefArrayToTempl = engine->GetEngineProperty(asEP_EXPAND_DEF_ARRAY_TO_TMPL) ? true : false;
	engine->SetEngineProperty(asEP_EXPAND_DEF_ARRAY_TO_TMPL, true);

	// Write enum types and their values
	fprintf(f, "// Enums\n");
	c = engine->GetEnumCount();
	for( n = 0; n < c; n++ )
	{
		int typeId;
		asDWORD accessMask;
		const char *nameSpace;
		const char *enumName = engine->GetEnumByIndex(n, &typeId, &nameSpace, 0, &accessMask);
		if( accessMask != currAccessMask )
		{
			fprintf(f, "access %X\n", (unsigned int)(accessMask));
			currAccessMask = accessMask;
		}
		if( nameSpace != currNamespace )
		{
			fprintf(f, "namespace %s\n", nameSpace);
			currNamespace = nameSpace;
		}
		fprintf(f, "enum %s\n", enumName);
		for( int m = 0; m < engine->GetEnumValueCount(typeId); m++ )
		{
			const char *valName;
			int val;
			valName = engine->GetEnumValueByIndex(typeId, m, &val);
			fprintf(f, "enumval %s %s %d\n", enumName, valName, val);
		}
	}

	// Enumerate all types
	fprintf(f, "\n// Types\n");

	c = engine->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		asIObjectType *type = engine->GetObjectTypeByIndex(n);
		asDWORD accessMask = type->GetAccessMask();
		if( accessMask != currAccessMask )
		{
			fprintf(f, "access %X\n", (unsigned int)(accessMask));
			currAccessMask = accessMask;
		}
		const char *nameSpace = type->GetNamespace();
		if( nameSpace != currNamespace )
		{
			fprintf(f, "namespace %s\n", nameSpace);
			currNamespace = nameSpace;
		}
		if( type->GetFlags() & asOBJ_SCRIPT_OBJECT )
		{
			// This should only be interfaces
			assert( type->GetSize() == 0 );

			fprintf(f, "intf %s\n", type->GetName());
		}
		else
		{
			// Only the type flags are necessary. The application flags are application 
			// specific and doesn't matter to the offline compiler. The object size is also
			// unnecessary for the offline compiler
			fprintf(f, "objtype \"%s\" %u\n", engine->GetTypeDeclaration(type->GetTypeId()), (unsigned int)(type->GetFlags() & 0xFF));
		}
	}

	c = engine->GetTypedefCount();
	for( n = 0; n < c; n++ )
	{
		int typeId;
		asDWORD accessMask;
		const char *nameSpace;
		const char *typeDef = engine->GetTypedefByIndex(n, &typeId, &nameSpace, 0, &accessMask);
		if( nameSpace != currNamespace )
		{
			fprintf(f, "namespace %s\n", nameSpace);
			currNamespace = nameSpace;
		}
		if( accessMask != currAccessMask )
		{
			fprintf(f, "access %X\n", (unsigned int)(accessMask));
			currAccessMask = accessMask;
		}
		fprintf(f, "typedef %s \"%s\"\n", typeDef, engine->GetTypeDeclaration(typeId));
	}

	c = engine->GetFuncdefCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *funcDef = engine->GetFuncdefByIndex(n);
		asDWORD accessMask = funcDef->GetAccessMask();
		const char *nameSpace = funcDef->GetNamespace();
		if( nameSpace != currNamespace )
		{
			fprintf(f, "namespace %s\n", nameSpace);
			currNamespace = nameSpace;
		}
		if( accessMask != currAccessMask )
		{
			fprintf(f, "access %X\n", (unsigned int)(accessMask));
			currAccessMask = accessMask;
		}
		fprintf(f, "funcdef \"%s\"\n", funcDef->GetDeclaration());
	}

	// Write the object types members
	fprintf(f, "\n// Type members\n");
	
	c = engine->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		asIObjectType *type = engine->GetObjectTypeByIndex(n);
		const char *nameSpace = type->GetNamespace();
		if( nameSpace != currNamespace )
		{
			fprintf(f, "namespace %s\n", nameSpace);
			currNamespace = nameSpace;
		}
		string typeDecl = engine->GetTypeDeclaration(type->GetTypeId());
		if( type->GetFlags() & asOBJ_SCRIPT_OBJECT )
		{
			for( asUINT m = 0; m < type->GetMethodCount(); m++ )
			{
				asIScriptFunction *func = type->GetMethodByIndex(m);
				asDWORD accessMask = func->GetAccessMask();
				if( accessMask != currAccessMask )
				{
					fprintf(f, "access %X\n", (unsigned int)(accessMask));
					currAccessMask = accessMask;
				}
				fprintf(f, "intfmthd %s \"%s\"\n", typeDecl.c_str(), func->GetDeclaration(false));
			}
		}
		else
		{
			asUINT m;
			for( m = 0; m < type->GetFactoryCount(); m++ )
			{
				asIScriptFunction *func = type->GetFactoryByIndex(m);
				asDWORD accessMask = func->GetAccessMask();
				if( accessMask != currAccessMask )
				{
					fprintf(f, "access %X\n", (unsigned int)(accessMask));
					currAccessMask = accessMask;
				}
				fprintf(f, "objbeh \"%s\" %d \"%s\"\n", typeDecl.c_str(), asBEHAVE_FACTORY, func->GetDeclaration(false));
			}
			for( m = 0; m < type->GetBehaviourCount(); m++ )
			{
				asEBehaviours beh;
				asIScriptFunction *func = type->GetBehaviourByIndex(m, &beh);
				fprintf(f, "objbeh \"%s\" %d \"%s\"\n", typeDecl.c_str(), beh, func->GetDeclaration(false));
			}
			for( m = 0; m < type->GetMethodCount(); m++ )
			{
				asIScriptFunction *func = type->GetMethodByIndex(m);
				asDWORD accessMask = func->GetAccessMask();
				if( accessMask != currAccessMask )
				{
					fprintf(f, "access %X\n", (unsigned int)(accessMask));
					currAccessMask = accessMask;
				}
				fprintf(f, "objmthd \"%s\" \"%s\"\n", typeDecl.c_str(), func->GetDeclaration(false));
			}
			for( m = 0; m < type->GetPropertyCount(); m++ )
			{
				asDWORD accessMask;
				type->GetProperty(m, 0, 0, 0, 0, 0, &accessMask);
				if( accessMask != currAccessMask )
				{
					fprintf(f, "access %X\n", (unsigned int)(accessMask));
					currAccessMask = accessMask;
				}
				fprintf(f, "objprop \"%s\" \"%s\"\n", typeDecl.c_str(), type->GetPropertyDeclaration(m));
			}
		}
	}

	// Write functions
	fprintf(f, "\n// Functions\n");

	c = engine->GetGlobalFunctionCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *func = engine->GetGlobalFunctionByIndex(n);
		const char *nameSpace = func->GetNamespace();
		if( nameSpace != currNamespace )
		{
			fprintf(f, "namespace %s\n", nameSpace);
			currNamespace = nameSpace;
		}
		asDWORD accessMask = func->GetAccessMask();
		if( accessMask != currAccessMask )
		{
			fprintf(f, "access %X\n", (unsigned int)(accessMask));
			currAccessMask = accessMask;
		}
		fprintf(f, "func \"%s\"\n", func->GetDeclaration());
	}

	// Write global properties
	fprintf(f, "\n// Properties\n");

	c = engine->GetGlobalPropertyCount();
	for( n = 0; n < c; n++ )
	{
		const char *name;
		int typeId;
		bool isConst;
		asDWORD accessMask;
		const char *nameSpace;
		engine->GetGlobalPropertyByIndex(n, &name, &nameSpace, &typeId, &isConst, 0, 0, &accessMask);
		if( accessMask != currAccessMask )
		{
			fprintf(f, "access %X\n", (unsigned int)(accessMask));
			currAccessMask = accessMask;
		}
		if( nameSpace != currNamespace )
		{
			fprintf(f, "namespace %s\n", nameSpace);
			currNamespace = nameSpace;
		}
		fprintf(f, "prop \"%s%s %s\"\n", isConst ? "const " : "", engine->GetTypeDeclaration(typeId), name);
	}

	// Write string factory
	fprintf(f, "\n// String factory\n");
	int typeId = engine->GetStringFactoryReturnTypeId();
	if( typeId > 0 )
		fprintf(f, "strfactory \"%s\"\n", engine->GetTypeDeclaration(typeId));

	// Write default array type
	fprintf(f, "\n// Default array type\n");
	typeId = engine->GetDefaultArrayTypeId();
	if( typeId > 0 )
		fprintf(f, "defarray \"%s\"\n", engine->GetTypeDeclaration(typeId));

	fclose(f);

	// Restore original settings
	engine->SetEngineProperty(asEP_EXPAND_DEF_ARRAY_TO_TMPL, expandDefArrayToTempl);

	return 0;
}

void PrintException(asIScriptContext *ctx, bool printStack)
{
	if( ctx->GetState() != asEXECUTION_EXCEPTION ) return;

	asIScriptEngine *engine = ctx->GetEngine();
	const asIScriptFunction *function = ctx->GetExceptionFunction();
	printf("func: %s\n", function->GetDeclaration());
	printf("modl: %s\n", function->GetModuleName());
	printf("sect: %s\n", function->GetScriptSectionName());
	printf("line: %d\n", ctx->GetExceptionLineNumber());
	printf("desc: %s\n", ctx->GetExceptionString());

	if( printStack )
	{
		printf("--- call stack ---\n");
		for( asUINT n = 1; n < ctx->GetCallstackSize(); n++ )
		{
			function = ctx->GetFunction(n);
			if( function )
			{
				if( function->GetFuncType() == asFUNC_SCRIPT )
				{
					printf("%s (%d): %s\n", function->GetScriptSectionName(),
											ctx->GetLineNumber(n),
											function->GetDeclaration());
				}
				else
				{
					// The context is being reused by the application for a nested call
					printf("{...application...}: %s\n", function->GetDeclaration());
				}
			}
			else
			{
				// The context is being reused by the script engine for a nested call
				printf("{...script engine...}\n");
			}			
		}
	}
}

END_AS_NAMESPACE
