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
#include "ScriptManager.h"
#include <new>
#include <random>


namespace spades {

	class MathScriptObjectRegistrar: public ScriptObjectRegistrar {
		static unsigned int GetRandomInt(unsigned int range) {
			switch(range){
				case 0:
					asGetActiveContext()->SetException("Invalid argument.");
					return 0;
				case 1:
					return 0;
				case 2:
					return SampleRandom() & 1;
				case 4:
					return SampleRandom() & 3;
				case 8:
					return SampleRandom() & 7;
				case 16:
					return SampleRandom() & 15;
				case 32:
					return SampleRandom() & 31;
				case 64:
					return SampleRandom() & 63;
			}
			// We can't directly use the engine here, we need a distribution

			// range - 1 because it's inclusive and we want exclusive
            return SampleRandomInt<unsigned int>(0, range - 1);
		}
		static unsigned int GetRandomUIntRange(unsigned int a,
											   unsigned int b){
			if(a > b) {
				std::swap(a, b); a++; b++;
			}
				
			return GetRandomInt(b - a) + a;
		}
		static int GetRandomIntRange(int a,
									 int b){
			if(a > b) {
				std::swap(a, b); a++; b++;
			}
			return (int)(GetRandomInt((unsigned int)(b - a)) + (unsigned int)a);
		}
		static float GetRandomFloatRange(float a, float b) {
			return SampleRandomFloat() * (b - a) + a;
		}
	public:
		MathScriptObjectRegistrar():
		ScriptObjectRegistrar("Math"){}
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			static float PiF = (float)M_PI;
			static double Pi = (double)M_PI;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("IntVector3",
												sizeof(IntVector3),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_ALLINTS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Vector2",
												sizeof(Vector2),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_ALLFLOATS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Vector3",
												sizeof(Vector3),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_ALLFLOATS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Vector4",
												sizeof(Vector4),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_ALLFLOATS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("Matrix4",
												sizeof(Matrix4),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK | asOBJ_APP_CLASS_ALLFLOATS);
					manager->CheckError(r);
					r = eng->RegisterObjectType("AABB2",
												sizeof(AABB2),
												asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK | asOBJ_APP_CLASS_ALLFLOATS);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterGlobalProperty("const float PiF", &PiF);
					manager->CheckError(r);
					r = eng->RegisterGlobalProperty("const double Pi", &Pi);
					manager->CheckError(r);
					
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
					
					// Register member variables
					r = eng->RegisterObjectProperty("IntVector3",
													"int x",
													asOFFSET(IntVector3, x));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("IntVector3",
													"int y",
													asOFFSET(IntVector3, y));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("IntVector3",
													"int z",
													asOFFSET(IntVector3, z));
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
					r = eng->RegisterObjectMethod("IntVector3",
												  "IntVector3 opNeg() const",
												  asMETHODPR(IntVector3, operator-, () const, IntVector3), asCALL_THISCALL);
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
					
					// Register member variables
					r = eng->RegisterObjectProperty("Vector2",
													"float x",
													asOFFSET(Vector3, x));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("Vector2",
													"float y",
													asOFFSET(Vector2, y));
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
												  "Vector2 &opAddAssign(float)",
												  asMETHODPR(Vector2, operator+=, (float), Vector2&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 &opSubAssign(float)",
												  asMETHODPR(Vector2, operator-=, (float), Vector2&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 &opMulAssign(float)",
												  asMETHODPR(Vector2, operator*=, (float), Vector2&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 &opDivAssign(float)",
												  asMETHODPR(Vector2, operator/=, (float), Vector2&), asCALL_THISCALL);
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
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opAdd(float) const",
												  asMETHODPR(Vector2, operator+, (float) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opSub(float) const",
												  asMETHODPR(Vector2, operator-, (float) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opMul(float) const",
												  asMETHODPR(Vector2, operator*, (float) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opDiv(float) const",
												  asMETHODPR(Vector2, operator/, (float) const, Vector2), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector2",
												  "Vector2 opNeg() const",
												  asMETHODPR(Vector2, operator-, () const, Vector2), asCALL_THISCALL);
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
												  "Vector2 get_Normalized() const",
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
					
					// Register member variables
					r = eng->RegisterObjectProperty("Vector3",
													"float x",
													asOFFSET(Vector3, x));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("Vector3",
													"float y",
													asOFFSET(Vector3, y));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("Vector3",
													"float z",
													asOFFSET(Vector3, z));
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
												  "Vector3 &opAddAssign(float)",
												  asMETHODPR(Vector3, operator+=, (float), Vector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 &opSubAssign(float)",
												  asMETHODPR(Vector3, operator-=, (float), Vector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 &opMulAssign(float)",
												  asMETHODPR(Vector3, operator*=, (float), Vector3&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 &opDivAssign(float)",
												  asMETHODPR(Vector3, operator/=, (float), Vector3&), asCALL_THISCALL);
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
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opAdd(float) const",
												  asMETHODPR(Vector3, operator+, (float) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opSub(float) const",
												  asMETHODPR(Vector3, operator-, (float) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opMul(float) const",
												  asMETHODPR(Vector3, operator*, (float) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opDiv(float) const",
												  asMETHODPR(Vector3, operator/, (float) const, Vector3), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector3",
												  "Vector3 opNeg() const",
												  asMETHODPR(Vector3, operator-, () const, Vector3), asCALL_THISCALL);
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
												  "Vector3 get_Normalized() const",
												  asMETHOD(Vector3, Normalize),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float Dot(const Vector3& in, const Vector3& in)",
													asFUNCTION(Vector3::Dot),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("Vector3 Cross(const Vector3& in, const Vector3& in)",
													asFUNCTION(Vector3::Cross),
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
					
					struct Vector4Funcs {
						static void Construct1(Vector4 *self) {
							new(self) Vector4();
						}
						static void Construct2(const Vector4& old, Vector4 *self) {
							new(self) Vector4(old);
						}
						static void Construct3(float x, float y, float z, float w, Vector4 *self) {
							new(self) Vector4();
							self->x = x; self->y = y; self->z = z; self->w = w;
						}
						static Vector4 Floor(const Vector4& v) {
							return Vector4::Make(floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w));
						}
						static Vector4 Ceil(const Vector4& v) {
							return Vector4::Make(ceilf(v.x), ceilf(v.y), ceilf(v.z), ceilf(v.w));
						}
					};
					// Register the constructors
					r = eng->RegisterObjectBehaviour("Vector4", asBEHAVE_CONSTRUCT,
													 "void f()",
													 asFUNCTION(Vector4Funcs::Construct1),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Vector4", asBEHAVE_CONSTRUCT,
													 "void f(const Vector4 &in)",
													 asFUNCTION(Vector4Funcs::Construct2),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Vector4", asBEHAVE_CONSTRUCT,
													 "void f(float, float, float, float)",
													 asFUNCTION(Vector4Funcs::Construct3),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					
					// Register member variables
					r = eng->RegisterObjectProperty("Vector4",
													"float x",
													asOFFSET(Vector4, x));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("Vector4",
													"float y",
													asOFFSET(Vector4, y));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("Vector4",
													"float z",
													asOFFSET(Vector4, z));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("Vector4",
													"float w",
													asOFFSET(Vector4, w));
					manager->CheckError(r);
					
					// Register the operator overloads
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opAddAssign(const Vector4 &in)",
												  asMETHODPR(Vector4, operator+=, (const Vector4 &), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opSubAssign(const Vector4 &in)",
												  asMETHODPR(Vector4, operator-=, (const Vector4 &), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opMulAssign(const Vector4 &in)",
												  asMETHODPR(Vector4, operator*=, (const Vector4 &), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opDivAssign(const Vector4 &in)",
												  asMETHODPR(Vector4, operator/=, (const Vector4 &), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opAddAssign(float)",
												  asMETHODPR(Vector4, operator+=, (float), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opSubAssign(float)",
												  asMETHODPR(Vector4, operator-=, (float), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opMulAssign(float)",
												  asMETHODPR(Vector4, operator*=, (float), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 &opDivAssign(float)",
												  asMETHODPR(Vector4, operator/=, (float), Vector4&), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "bool opEquals(const Vector4 &in) const",
												  asMETHODPR(Vector4, operator==, (const Vector4 &) const, bool), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opAdd(const Vector4 &in) const",
												  asMETHODPR(Vector4, operator+, (const Vector4 &) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opSub(const Vector4 &in) const",
												  asMETHODPR(Vector4, operator-, (const Vector4 &) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opMul(const Vector4 &in) const",
												  asMETHODPR(Vector4, operator*, (const Vector4 &) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opDiv(const Vector4 &in) const",
												  asMETHODPR(Vector4, operator/, (const Vector4 &) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opAdd(float) const",
												  asMETHODPR(Vector4, operator+, (float) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opSub(float) const",
												  asMETHODPR(Vector4, operator-, (float) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opMul(float) const",
												  asMETHODPR(Vector4, operator*, (float) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opDiv(float) const",
												  asMETHODPR(Vector4, operator/, (float) const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 opNeg() const",
												  asMETHODPR(Vector4, operator-, () const, Vector4), asCALL_THISCALL);
					manager->CheckError(r);
					
					// Register the object methods
					r = eng->RegisterObjectMethod("Vector4",
												  "float get_Length() const",
												  asMETHOD(Vector4, GetLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector4",
												  "float get_LengthPowered() const",
												  asMETHOD(Vector4, GetPoweredLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector4",
												  "float get_ManhattanLength() const",
												  asMETHOD(Vector4, GetManhattanLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector4",
												  "float get_ChebyshevLength() const",
												  asMETHOD(Vector4, GetChebyshevLength),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Vector4",
												  "Vector4 get_Normalized() const",
												  asMETHOD(Vector4, Normalize),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float Dot(const Vector4& in, const Vector4& in)",
													asFUNCTION(Vector4::Dot),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector4 Floor(const Vector4& in)",
													asFUNCTION(Vector4Funcs::Floor),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector4 Ceil(const Vector4& in)",
													asFUNCTION(Vector4Funcs::Ceil),
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
						static Vector3 Transform3(const Vector3& vec, Matrix4 *self) {
							return (*self * vec).GetXYZ();
						}
						static Vector4 Transform4(const Vector4& vec, Matrix4 *self) {
							return *self * vec;
						}
						static Matrix4 Translate(const Vector3& vec) {
							return Matrix4::Translate(vec);
						}
						static Matrix4 Rotate(const Vector3& axis, float rad) {
							return Matrix4::Rotate(axis, rad);
						}
						static Matrix4 Scale(const Vector3& vec) {
							return Matrix4::Scale(vec);
						}
						static Matrix4 FromAxes(const Vector3& v1,
												const Vector3& v2,
												const Vector3& v3,
												const Vector3& v4) {
							return Matrix4::FromAxis(v1, v2, v3, v4);
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
					r = eng->RegisterObjectMethod("Matrix4",
												  "Vector4 opMul(const Vector4 &in) const",
												  asFUNCTION(Matrix4Funcs::Transform4), asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Matrix4",
												  "Vector3 opMul(const Vector3 &in) const",
												  asFUNCTION(Matrix4Funcs::Transform3), asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					
					// Register the object methods
					r = eng->RegisterObjectMethod("Matrix4",
												  "Matrix4 get_Transposed() const",
												  asMETHOD(Matrix4, Transposed),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Matrix4",
												  "Matrix4 get_Inverted() const",
												  asMETHOD(Matrix4, Inversed),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					r = eng->RegisterObjectMethod("Matrix4",
												  "float get_InvertedFast() const",
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
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateTranslateMatrix(const Vector3& in)",
													asFUNCTION(Matrix4Funcs::Translate),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateTranslateMatrix(float, float, float)",
													asFUNCTIONPR(Matrix4::Translate, (float,float,float), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateRotateMatrix(const Vector3& in, float)",
													asFUNCTION(Matrix4Funcs::Rotate),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateScaleMatrix(float)",
													asFUNCTIONPR(Matrix4::Scale, (float), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateScaleMatrix(const Vector3& in)",
													asFUNCTION(Matrix4Funcs::Scale),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateScaleMatrix(float, float, float)",
													asFUNCTIONPR(Matrix4::Scale, (float,float,float), Matrix4),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Matrix4 CreateMatrixFromAxes(const Vector3&in, const Vector3&in, const Vector3&in, const Vector3& in)",
													asFUNCTION(Matrix4Funcs::FromAxes),
													asCALL_CDECL);
					manager->CheckError(r);
					struct AABB2Funcs {
						static void Construct1(AABB2 *self) {
							new(self) AABB2();
						}
						static void Construct2(const AABB2& old, AABB2 *self) {
							new(self) AABB2(old);
						}
						static void Construct3(float x, float y, float w, float h, AABB2 *self) {
							new(self) AABB2(x, y, w, h);
						}
						static void Construct4(Vector2 minVec, Vector2 maxVec, AABB2 *self) {
							new(self) AABB2(minVec, maxVec);
						}
					};
					r = eng->RegisterObjectProperty("AABB2",
													"Vector2 min",
													asOFFSET(AABB2, min));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("AABB2",
													"Vector2 max",
													asOFFSET(AABB2, max));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("AABB2",
													"float minX",
													asOFFSET(AABB2, min.x));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("AABB2",
													"float minY",
													asOFFSET(AABB2, min.y));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("AABB2",
													"float maxX",
													asOFFSET(AABB2, max.x));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("AABB2",
													"float maxY",
													asOFFSET(AABB2, max.y));
					manager->CheckError(r);
					
					// Register the constructors
					r = eng->RegisterObjectBehaviour("AABB2", asBEHAVE_CONSTRUCT,
													 "void f()",
													 asFUNCTION(AABB2Funcs::Construct1),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("AABB2", asBEHAVE_CONSTRUCT,
													 "void f(const AABB2 &in)",
													 asFUNCTION(AABB2Funcs::Construct2),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("AABB2", asBEHAVE_CONSTRUCT,
													 "void f(float, float, float, float)",
													 asFUNCTION(AABB2Funcs::Construct3),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("AABB2", asBEHAVE_CONSTRUCT,
													 "void f(Vector2, Vector2)",
													 asFUNCTION(AABB2Funcs::Construct4),
													 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					
					// Register the operator overloads
					r = eng->RegisterObjectMethod("AABB2",
												  "bool Contains(const Vector2 &in)",
												  asMETHOD(AABB2, Contains), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("AABB2",
												  "bool Intersects(const AABB2 &in)",
												  asMETHOD(AABB2, Intersects), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("AABB2",
												  "void Add(const Vector2& in)",
												  asMETHODPR(AABB2, operator+=, (const Vector2 &), void), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("AABB2",
												  "void Add(const AABB2& in)",
												  asMETHODPR(AABB2, operator+=, (const AABB2 &), void), asCALL_THISCALL);
					manager->CheckError(r);
					

					/*** Other Global Functions ***/
					
					struct UtilFuncs {
						static std::string ToString(int i) {
							char buf[256];
							sprintf(buf, "%d", i);
							return buf;
						}
						static std::string ToString(double i) {
							char buf[256];
							sprintf(buf, "%f", i);
							return buf;
						}
						static int ParseInt(const std::string& s) {
							try{
								return std::stoi(s);
							}catch(...){
								return 0;
							}
						}
						static double ParseDouble(const std::string& s) {
							try{
								return std::stod(s);
							}catch(...){
								return 0.0;
							}
						}
					};
					
					r = eng->RegisterGlobalFunction("string Replace(const string&in, const string& in, const string&in)",
													asFUNCTION(Replace),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("string TrimSpaces(const string&in)",
													asFUNCTION(TrimSpaces),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float GetRandom()",
													asFUNCTION(SampleRandomFloat),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("uint GetRandom(uint)",
													asFUNCTION(GetRandomInt),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("uint GetRandom(uint, uint)",
													asFUNCTION(GetRandomUIntRange),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("int GetRandom(int, int)",
													asFUNCTION(GetRandomIntRange),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float GetRandom(float, float)",
													asFUNCTION(GetRandomFloatRange),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float Mix(float,float,float)",
													asFUNCTIONPR(Mix, (float,float,float), float),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector2 Mix(const Vector2& in,const Vector2& in,float)",
													asFUNCTIONPR(Mix, (const Vector2&,const Vector2&,float), Vector2),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("Vector3 Mix(const Vector3& in,const Vector3& in,float)",
													asFUNCTIONPR(Mix, (const Vector3&,const Vector3&,float), Vector3),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("float SmoothStep(float)",
													asFUNCTION(SmoothStep),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("string ToString(int)",
													asFUNCTIONPR(UtilFuncs::ToString, (int), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					
					r = eng->RegisterGlobalFunction("string ToString(double)",
													asFUNCTIONPR(UtilFuncs::ToString, (double), std::string),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("int ParseInt(const string& in)",
													asFUNCTION(UtilFuncs::ParseInt),
													asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterGlobalFunction("double ParseDouble(const string& in)",
													asFUNCTION(UtilFuncs::ParseDouble),
													asCALL_CDECL);
					manager->CheckError(r);
					
					
					break;
				default: break;
			}
			
			
			
		}
	};
	
	static MathScriptObjectRegistrar registrar;
}
