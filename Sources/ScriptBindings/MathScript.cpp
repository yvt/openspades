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

#include <Core/Math.h>
#include <stdlib.h>
#include "ScriptManager.h"
#include <new>

namespace spades {

	class MathScriptObjectRegistrar: public ScriptObjectRegistrar {
	public:
		MathScriptObjectRegistrar():
		ScriptObjectRegistrar("Math"){}
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("IntVector3",
												sizeof(IntVector3),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK | asOBJ_APP_CLASS_ALLINTS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Vector2",
												sizeof(Vector2),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK | asOBJ_APP_CLASS_ALLINTS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Vector3",
												sizeof(Vector3),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK | asOBJ_APP_CLASS_ALLINTS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Vector4",
												sizeof(Vector4),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK | asOBJ_APP_CLASS_ALLINTS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Matrix4",
												sizeof(Matrix4),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK | asOBJ_APP_CLASS_ALLINTS);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					struct IntVector3Funcs {
						static void Construct1(IntVector3 *self) {
							new(self) IntVector3();
						}
						static void Construct2(const IntVector3& old, IntVector3 *self) {
							new(self) IntVector3(old);
						}
						static void Construct3(int x, int y, int z, IntVector3 *self) {
							new(self) IntVector3();
							self->x = x; self->y = y; self->z = z;
						}
						static void Construct4(const Vector3& old, IntVector3 *self) {
							new(self) IntVector3();
							self->x = (int)old.x; self->y = (int)old.y; self->z = (int)old.z;
						}
					};
					// Register the constructors
					r = eng->RegisterObjectBehaviour("IntVector3", asBEHAVE_CONSTRUCT,
														"void f()",
													 asFUNCTION(IntVector3Funcs::Construct1),
														asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("IntVector3", asBEHAVE_CONSTRUCT,
														"void f(const IntVector3 &in)",
														asFUNCTION(IntVector3Funcs::Construct2),
														asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("IntVector3", asBEHAVE_CONSTRUCT,
													 "void f(int, int, int)",
													 asFUNCTION(IntVector3Funcs::Construct3),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("IntVector3", asBEHAVE_CONSTRUCT,
													 "void f(const Vector3 &in)",
													 asFUNCTION(IntVector3Funcs::Construct4),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					
					// Register the operator overloads
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 &opAddAssign(const IntVector3 &in)",
												  asMETHODPR(IntVector3, operator+=, (const IntVector3 &), IntVector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 &opSubAssign(const IntVector3 &in)",
												  asMETHODPR(IntVector3, operator-=, (const IntVector3 &), IntVector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 &opMulAssign(const IntVector3 &in)",
												  asMETHODPR(IntVector3, operator*=, (const IntVector3 &), IntVector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 &opDivAssign(const IntVector3 &in)",
												  asMETHODPR(IntVector3, operator/=, (const IntVector3 &), IntVector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "bool opEquals(const IntVector3 &in) const",
												  asMETHODPR(IntVector3, operator==, (const IntVector3 &) const, bool), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 opAdd(const IntVector3 &in) const",
												  asMETHODPR(IntVector3, operator+, (const IntVector3 &) const, IntVector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 opSub(const IntVector3 &in) const",
												  asMETHODPR(IntVector3, operator-, (const IntVector3 &) const, IntVector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 opMul(const IntVector3 &in) const",
												  asMETHODPR(IntVector3, operator*, (const IntVector3 &) const, IntVector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 opDiv(const IntVector3 &in) const",
												  asMETHODPR(IntVector3, operator/, (const IntVector3 &) const, IntVector3), asCALL_THISCALL);
					manager->CheckError(r);
					
					// Register the object methods
					r = eng->RegisterObjectMethod("IntVector3",
												  "int get_ManhattanLength() const",
												  asMETHOD(IntVector3, GetManhattanLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("IntVector3",
												  "int get_ChebyshevLength() const",
												  asMETHOD(IntVector3, GetChebyshevLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("int Dot(const IntVector3& in, const IntVector3& in)",
													asFUNCTION(IntVector3::Dot),
													asCALL_CDECL);
					manager->CheckError(r);
					
					
					struct Vector2Funcs {
						static void Construct1(Vector2 *self) {
							new(self) Vector2();
						}
						static void Construct2(const Vector2& old, Vector2 *self) {
							new(self) Vector2(old);
						}
						static void Construct3(float x, float y, Vector2 *self) {
							new(self) Vector2();
							self->x = x; self->y = y;
						}
					};
					// Register the constructors
					r = eng->RegisterObjectBehaviour("Vector2", asBEHAVE_CONSTRUCT,
													 "void f()",
													 asFUNCTION(Vector2Funcs::Construct1),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Vector2", asBEHAVE_CONSTRUCT,
													 "void f(const Vector2 &in)",
													 asFUNCTION(Vector2Funcs::Construct2),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Vector2", asBEHAVE_CONSTRUCT,
													 "void f(float, float)",
													 asFUNCTION(Vector2Funcs::Construct3),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					
					// Register the operator overloads
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 &opAddAssign(const Vector2 &in)",
												  asMETHODPR(Vector2, operator+=, (const Vector2 &), Vector2&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 &opSubAssign(const Vector2 &in)",
												  asMETHODPR(Vector2, operator-=, (const Vector2 &), Vector2&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 &opMulAssign(const Vector2 &in)",
												  asMETHODPR(Vector2, operator*=, (const Vector2 &), Vector2&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 &opDivAssign(const Vector2 &in)",
												  asMETHODPR(Vector2, operator/=, (const Vector2 &), Vector2&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "bool opEquals(const Vector2 &in) const",
												  asMETHODPR(Vector2, operator==, (const Vector2 &) const, bool), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opAdd(const Vector2 &in) const",
												  asMETHODPR(Vector2, operator+, (const Vector2 &) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opSub(const Vector2 &in) const",
												  asMETHODPR(Vector2, operator-, (const Vector2 &) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opMul(const Vector2 &in) const",
												  asMETHODPR(Vector2, operator*, (const Vector2 &) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opDiv(const Vector2 &in) const",
												  asMETHODPR(Vector2, operator/, (const Vector2 &) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					
					// Register the object methods
					r = eng->RegisterObjectMethod("Vector2",
												  "float get_Length() const",
												  asMETHOD(Vector2, GetLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector2",
												  "float get_LengthPowered() const",
												  asMETHOD(Vector2, GetPoweredLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector2",
												  "float get_ManhattanLength() const",
												  asMETHOD(Vector2, GetManhattanLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector2",
												  "float get_ChebyshevLength() const",
												  asMETHOD(Vector2, GetChebyshevLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector2",
												  "float get_Normalized() const",
												  asMETHOD(Vector2, Normalize),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float Dot(const Vector2& in, const Vector2& in)",
													asFUNCTION(Vector2::Dot),
													asCALL_CDECL);
					manager->CheckError(r);
					
					struct Vector3Funcs {
						static void Construct1(Vector3 *self) {
							new(self) Vector3();
						}
						static void Construct2(const Vector3& old, Vector3 *self) {
							new(self) Vector3(old);
						}
						static void Construct3(float x, float y, float z, Vector3 *self) {
							new(self) Vector3();
							self->x = x; self->y = y; self->z = z;
						}
						static void Construct4(const IntVector3& old, Vector3 *self) {
							new(self) Vector3();
							self->x = old.x; self->y = old.y; self->z = old.z;
						}
						static Vector3 Floor(const Vector3& v) {
							return Vector3::Make(floorf(v.x), floorf(v.y), floorf(v.z));
						}
						static Vector3 Ceil(const Vector3& v) {
							return Vector3::Make(ceilf(v.x), ceilf(v.y), ceilf(v.z));
						}
					};
					// Register the constructors
					r = eng->RegisterObjectBehaviour("Vector3", asBEHAVE_CONSTRUCT,
													 "void f()",
													 asFUNCTION(Vector3Funcs::Construct1),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Vector3", asBEHAVE_CONSTRUCT,
													 "void f(const Vector3 &in)",
													 asFUNCTION(Vector3Funcs::Construct2),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Vector3", asBEHAVE_CONSTRUCT,
													 "void f(float, float, float)",
													 asFUNCTION(Vector3Funcs::Construct3),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Vector3", asBEHAVE_CONSTRUCT,
													 "void f(const IntVector3&in)",
													 asFUNCTION(Vector3Funcs::Construct4),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					
					// Register the operator overloads
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 &opAddAssign(const Vector3 &in)",
												  asMETHODPR(Vector3, operator+=, (const Vector3 &), Vector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 &opSubAssign(const Vector3 &in)",
												  asMETHODPR(Vector3, operator-=, (const Vector3 &), Vector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 &opMulAssign(const Vector3 &in)",
												  asMETHODPR(Vector3, operator*=, (const Vector3 &), Vector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 &opDivAssign(const Vector3 &in)",
												  asMETHODPR(Vector3, operator/=, (const Vector3 &), Vector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "bool opEquals(const Vector3 &in) const",
												  asMETHODPR(Vector3, operator==, (const Vector3 &) const, bool), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opAdd(const Vector3 &in) const",
												  asMETHODPR(Vector3, operator+, (const Vector3 &) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opSub(const Vector3 &in) const",
												  asMETHODPR(Vector3, operator-, (const Vector3 &) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opMul(const Vector3 &in) const",
												  asMETHODPR(Vector3, operator*, (const Vector3 &) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opDiv(const Vector3 &in) const",
												  asMETHODPR(Vector3, operator/, (const Vector3 &) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					
					// Register the object methods
					r = eng->RegisterObjectMethod("Vector3",
												  "float get_Length() const",
												  asMETHOD(Vector3, GetLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector3",
												  "float get_LengthPowered() const",
												  asMETHOD(Vector3, GetPoweredLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector3",
												  "float get_ManhattanLength() const",
												  asMETHOD(Vector3, GetManhattanLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector3",
												  "float get_ChebyshevLength() const",
												  asMETHOD(Vector3, GetChebyshevLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector3",
												  "float get_Normalized() const",
												  asMETHOD(Vector3, Normalize),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float Dot(const Vector3& in, const Vector3& in)",
													asFUNCTION(Vector3::Dot),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector3 Floor(const Vector3& in)",
													asFUNCTION(Vector3Funcs::Floor),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector3 Ceil(const Vector3& in)",
													asFUNCTION(Vector3Funcs::Ceil),
													asCALL_CDECL);
					manager->CheckError(r);
					
					
					struct Matrix4Funcs {
						static void Construct1(Matrix4 *self) {
							new(self) Matrix4();
							*self = Matrix4::Identity();
						}
						static void Construct2(const Matrix4& old, Matrix4 *self) {
							new(self) Matrix4(old);
						}
						static void Construct3(float m00, float m10, float m20, float m30,
											   float m01, float m11, float m21, float m31,
											   float m02, float m12, float m22, float m32,
											   float m03, float m13, float m23, float m33,
											   Matrix4 *self) {
							new(self) Matrix4(m00, m10, m20, m30,
											  m01, m11, m21, m31,
											  m02, m12, m22, m32,
											  m03, m13, m23, m33);
						}
					};
					// Register the constructors
					r = eng->RegisterObjectBehaviour("Matrix4", asBEHAVE_CONSTRUCT,
													 "void f()",
													 asFUNCTION(Matrix4Funcs::Construct1),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Matrix4", asBEHAVE_CONSTRUCT,
													 "void f(const Matrix4 &in)",
													 asFUNCTION(Matrix4Funcs::Construct2),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Matrix4", asBEHAVE_CONSTRUCT,
													 "void f(float, float, float, float,"
													 "float, float, float, float,"
													 "float, float, float, float,"
													 "float, float, float, float)",
													 asFUNCTION(Matrix4Funcs::Construct3),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					
					// Register the operator overloads
					r = eng->RegisterObjectMethod("Matrix4",
												  "Matrix4 &opMulAssign(const Matrix4 &in)",
												  asMETHODPR(Matrix4, operator*=, (const Matrix4 &), Matrix4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Matrix4",
												  "Matrix4 opMul(const Matrix4 &in) const",
												  asMETHODPR(Matrix4, operator*, (const Matrix4 &) const, Matrix4), asCALL_THISCALL);
					manager->CheckError(r);
					
					// Register the object methods
					r = eng->RegisterObjectMethod("Matrix4",
												  "Matrix4 get_Transposed() const",
												  asMETHOD(Matrix4, Transposed),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Matrix4",
												  "Matrix4 get_Inversed() const",
												  asMETHOD(Matrix4, Inversed),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Matrix4",
												  "float get_InversedFast() const",
												  asMETHOD(Matrix4, InversedFast),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Matrix4",
												  "Vector3 GetOrigin() const",
												  asMETHOD(Matrix4, GetOrigin),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Matrix4",
												  "Vector3 GetAxis(int) const",
												  asMETHOD(Matrix4, GetAxis),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateTranslateMatrix(Vector3)",
													asFUNCTIONPR(Matrix4::Translate, (Vector3), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateTranslateMatrix(float, float, float)",
													asFUNCTIONPR(Matrix4::Translate, (float,float,float), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateRotateMatrix(Vector3, float)",
													asFUNCTION(Matrix4::Rotate),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateScaleMatrix(float)",
													asFUNCTIONPR(Matrix4::Scale, (float), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateScaleMatrix(Vector3)",
													asFUNCTIONPR(Matrix4::Scale, (Vector3), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateScaleMatrix(float, float, float)",
													asFUNCTIONPR(Matrix4::Scale, (float,float,float), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateMatrixFromAxes(Vector3,Vector3,Vector3,Vector3)",
													asFUNCTION(Matrix4::FromAxis),
													asCALL_CDECL);
					manager->CheckError(r);
					
					/*** Other Global Functions ***/
					
					r = eng->RegisterGlobalFunction("string Replace(const string&in, const string& in, const string&in)",
													asFUNCTION(Replace),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("string TrimSpaces(const string&in)",
													asFUNCTION(TrimSpaces),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float GetRandom()",
													asFUNCTION(GetRandom),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float Mix(float,float,float)",
													asFUNCTIONPR(Mix, (float,float,float), float),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector2 Mix(Vector2,Vector2,Vector2)",
													asFUNCTIONPR(Mix, (Vector2,Vector2,float), Vector2),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector3 Mix(Vector3,Vector3,Vector3)",
													asFUNCTIONPR(Mix, (Vector3,Vector3,float), Vector3),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float SmoothStep(float)",
													asFUNCTION(SmoothStep),
													asCALL_CDECL);
					manager->CheckError(r);
					
					
					break;
			}
			
			
			
		}
	};
	
	static MathScriptObjectRegistrar registrar;
}
