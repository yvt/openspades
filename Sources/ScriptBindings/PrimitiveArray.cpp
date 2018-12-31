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
#include <atomic>
#include <vector>
#include <Core/Debug.h>
#include <Core/Math.h>
#include <algorithm>
#include <stdint.h>

namespace spades {
	template <typename T>
	class PrimitiveArray {
		std::vector<T> inner;
		std::atomic<int> refCount {1};
		bool gcFlag = false;
	public:
		static asITypeInfo *scrType;
		typedef PrimitiveArray<T> ArrayType;
		static ArrayType *Factory1() {
			ArrayType *obj = new ArrayType();
			if(scrType->GetFlags() & asOBJ_GC)
				asGetActiveContext()->GetEngine()->NotifyGarbageCollectorOfNewObject(obj, scrType);
			return obj;
		}
		static ArrayType *Factory2(asUINT initialSize) {
			if(initialSize > 1024 * 1024 * 256) {
				asGetActiveContext()->SetException("Too many array elements");
				return NULL;
			}
			ArrayType *obj = new ArrayType(initialSize);
			if(scrType->GetFlags() & asOBJ_GC)
				asGetActiveContext()->GetEngine()->NotifyGarbageCollectorOfNewObject(obj, scrType);
			return obj;
		}
		static ArrayType *Factory3(asUINT initialSize, T initialValue) {
			if(initialSize > 1024 * 1024 * 256) {
				asGetActiveContext()->SetException("Too many array elements");
				return NULL;
			}
			ArrayType *obj = new ArrayType(initialSize, initialValue);
			if(scrType->GetFlags() & asOBJ_GC)
				asGetActiveContext()->GetEngine()->NotifyGarbageCollectorOfNewObject(obj, scrType);
			return obj;
		}
		static ArrayType *Factory4(void *initList) {
			asUINT length = *reinterpret_cast<const asUINT *>(initList);
			if(length > 1024 * 1024 * 256) {
				asGetActiveContext()->SetException("Too many array elements");
			}
			ArrayType *obj = new ArrayType(initList);
			if(scrType->GetFlags() & asOBJ_GC)
				asGetActiveContext()->GetEngine()->NotifyGarbageCollectorOfNewObject(obj, scrType);
			return obj;
		}

		PrimitiveArray(asUINT initialSize = 0) {
			inner.resize(initialSize);
		}
		PrimitiveArray(asUINT initialSize, T initialValue) {
			inner.resize(initialSize, initialValue);
		}
		PrimitiveArray(void *initList) {
			asUINT length = *reinterpret_cast<const asUINT *>(initList);
			inner.resize(length);
			memcpy(inner.data(), reinterpret_cast<const asUINT *>(initList) + 1, inner.size() * sizeof(T));
		}

		void AddRef() {
			gcFlag = false;
			refCount.fetch_add(1);
		}
		void Release() {
			gcFlag = false;

			if(refCount.fetch_sub(1) == 1) {
				delete this;
			}
		}
		void SetGCFlag() {
			gcFlag = true;
		}
		bool GetGCFlag() {
			return gcFlag;
		}
		int GetRefCount() {
			return refCount;
		}
		void EnumReferences(asIScriptEngine *eng) {}
		void ReleaseAllReferences(asIScriptEngine *eng){}

		T& At(asUINT index) {
			if(index >= inner.size()){
				asGetActiveContext()->SetException("Index out of bounds");

				// Cannot return `nullptr` because that will result in an undefined behavior
				static T dummyValue;
				return dummyValue;
			}
			return inner[index];
		}
		const T& At(asUINT index) const {
			if(index >= inner.size()){
				asGetActiveContext()->SetException("Index out of bounds");

				// Cannot return `nullptr` because that will result in an undefined behavior
				static T dummyValue;
				return dummyValue;
			}
			return inner[index];
		}
		ArrayType& operator =(const ArrayType& other) {
			if(&other == this) return *this;
			inner = other.inner;
			return *this;
		}
		void InsertAt(asUINT index, const T& val){
			if(index > inner.size()){
				asGetActiveContext()->SetException("Index out of bounds");
			}else{
				inner.insert(inner.begin() + index, val);
			}
		}
		void RemoveAt(asUINT index){
			if(index >= inner.size()){
				asGetActiveContext()->SetException("Index out of bounds");
			}else{
				inner.erase(inner.begin() + index);
			}
		}
		void InsertLast(const T& val) {
			inner.push_back(val);
		}
		void RemoveLast() {
			if(inner.empty()){
				asGetActiveContext()->SetException("No elements");
			}else{
				inner.pop_back();
			}
		}
		asUINT GetSize() const {
			return (asUINT)inner.size();
		}
		void Resize(asUINT siz) {
			if(siz > 1024 * 1024 * 1024){
				asGetActiveContext()->SetException("Too many array elements");
			}else{
				inner.resize((size_t)siz);
			}
		}
		void Reserve(asUINT siz) {
			if(siz > 1024 * 1024 * 1024){
				asGetActiveContext()->SetException("Too many array elements");
			}else{
				inner.reserve((size_t)siz);
			}
		}

		void SortAsc(asUINT index, asUINT count){
			if(count <= 0) return;
			if(index + count > inner.size()){
				asGetActiveContext()->SetException("Index out of bounds");
			}
			std::sort(inner.begin() + index, inner.begin() + index + count);
		}
		void SortDesc(asUINT index, asUINT count){
			if(count <= 0) return;
			if(index + count > inner.size()){
				asGetActiveContext()->SetException("Index out of bounds");
			}
			struct Sorter{
				static bool Sort(const T& a, const T& b) {
					return a > b;
				}
			};
			std::sort(inner.begin() + index, inner.begin() + index + count, Sorter::Sort);
		}
		void SortAsc() {
			SortAsc(0, GetSize());
		}
		void SortDesc() {
			SortDesc(0, GetSize());
		}

		void Reverse() {
			std::reverse(inner.begin(), inner.end());
		}

		int Find(const T& val) const {
			typename std::vector<T>::const_iterator it = std::find(inner.begin(), inner.end(), val);
			if(it == inner.end()) return -1;
			return static_cast<int> (it - inner.begin());
		}

		int Find(asUINT ind, const T& val) const {
			if(ind >= GetSize()) return -1;
			typename std::vector<T>::const_iterator it = std::find(inner.begin() + ind, inner.end(), val);
			if(it == inner.end()) return -1;
			return static_cast<int> (it - inner.begin());
		}

		bool operator ==(const ArrayType& array) const {
			if(this == &array)
				return true;
			if(GetSize() != array.GetSize())
				return false;
			size_t cnt = inner.size();
			for(size_t i = 0; i < cnt; i++)
				if(inner[i] != array.inner[i])
					return false;
			return true;
		}

		bool IsEmpty() const {
			return inner.empty();
		}

	};

	template<typename T>
	class PrimitiveArrayRegistrar: public ScriptObjectRegistrar {
		std::string typeName;
		std::string arrayTypeName;
		std::string tmpStr[4];
		int tmpId;
		typedef PrimitiveArray<T> ArrayType;
		const char *TN() {
			return typeName.c_str();
		}
		const char *ATN() {
			return arrayTypeName.c_str();
		}
		const char *F(const char *s) {
			tmpId = (tmpId + 1) & 3;
			tmpStr[tmpId] = Replace(s, "%s", typeName);
			return tmpStr[tmpId].c_str();
		}
	public:
		PrimitiveArrayRegistrar(const char *nam):
		ScriptObjectRegistrar(nam),
		typeName(nam),
		tmpId(0){
			arrayTypeName = F("array<%s>");
		}
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType(ATN(),
											0,
											asOBJ_REF | asOBJ_GC);
					ArrayType::scrType = eng->GetTypeInfoByDecl(ATN());
					SPAssert(ArrayType::scrType);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_FACTORY,
														F("array<%s>@ f()"),
														asFUNCTION(ArrayType::Factory1), asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_FACTORY,
													 F("array<%s>@ f(uint)"),
													 asFUNCTION(ArrayType::Factory2), asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_FACTORY,
													 F("array<%s>@ f(uint, %s)"),
													 asFUNCTION(ArrayType::Factory3), asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_LIST_FACTORY,
													 F("array<%s>@ f(int&in list) {repeat %s}"),
													 asFUNCTION(ArrayType::Factory4), asCALL_CDECL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_ADDREF,
													 F("void f()"),
													 asMETHOD(ArrayType, AddRef), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_RELEASE,
													 F("void f()"),
													 asMETHOD(ArrayType, Release), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_SETGCFLAG,
													 F("void f()"),
													 asMETHOD(ArrayType, SetGCFlag), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_GETGCFLAG,
													 F("bool f()"),
													 asMETHOD(ArrayType, GetGCFlag), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_GETREFCOUNT,
													 F("int f()"),
													 asMETHOD(ArrayType, GetRefCount), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_ENUMREFS,
													 F("void f(int& in)"),
													 asMETHOD(ArrayType, EnumReferences), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(ATN(), asBEHAVE_RELEASEREFS,
													 F("void f(int& in)"),
													 asMETHOD(ArrayType, ReleaseAllReferences), asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("%s& opIndex(uint)"),
												  asMETHODPR(ArrayType, At, (asUINT), T&),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("const %s& opIndex(uint) const"),
												  asMETHODPR(ArrayType, At, (asUINT) const, const T&),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("array<%s> &opAssign(const array<%s>&)"),
												  asMETHODPR(ArrayType, operator =, (const ArrayType&), ArrayType&),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void insertAt(uint, const %s& in)"),
												  asMETHODPR(ArrayType, InsertAt, (asUINT, const T&), void),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void removeAt(uint)"),
												  asMETHOD(ArrayType, RemoveAt),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void insertLast(const %s& in)"),
												  asMETHOD(ArrayType, InsertLast),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void removeLast()"),
												  asMETHOD(ArrayType, RemoveLast),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("uint length()"),
												  asMETHOD(ArrayType, GetSize),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void resize(uint)"),
												  asMETHOD(ArrayType, Resize),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void reserve(uint)"),
												  asMETHOD(ArrayType, Reserve),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void sortAsc()"),
												  asMETHODPR(ArrayType, SortAsc, (), void),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void sortAsc(uint, uint)"),
												  asMETHODPR(ArrayType, SortAsc, (asUINT, asUINT), void),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void sortDesc()"),
												  asMETHODPR(ArrayType, SortDesc, (), void),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void sortDesc(uint, uint)"),
												  asMETHODPR(ArrayType, SortDesc, (asUINT, asUINT), void),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("int find(const %s& in) const"),
												  asMETHODPR(ArrayType, Find, (const T&) const, int),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("int find(uint, const %s& in) const"),
												  asMETHODPR(ArrayType, Find, (asUINT, const T&) const, int),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("bool opEquals(const array<%s>&) const"),
												  asMETHODPR(ArrayType, operator ==, (const ArrayType&) const, bool),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("uint get_length()"),
												  asMETHOD(ArrayType, GetSize),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void set_length(uint)"),
												  asMETHOD(ArrayType, Resize),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("bool isEmpty()"),
												  asMETHOD(ArrayType, IsEmpty),
												  asCALL_THISCALL);
					manager->CheckError(r);

					// STL name

					r = eng->RegisterObjectMethod(ATN(),
												  F("uint size()"),
												  asMETHOD(ArrayType, GetSize),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("bool empty()"),
												  asMETHOD(ArrayType, IsEmpty),
												  asCALL_THISCALL);
					manager->CheckError(r);


					r = eng->RegisterObjectMethod(ATN(),
												  F("void push_back(const %s& in)"),
												  asMETHOD(ArrayType, InsertLast),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void pop_back()"),
												  asMETHOD(ArrayType, RemoveLast),
												  asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(ATN(),
												  F("void erase(uint)"),
												  asMETHOD(ArrayType, RemoveAt),
												  asCALL_THISCALL);
					manager->CheckError(r);
					break;
				default:
					break;
			}
		}
	};
	template<typename T> asITypeInfo *PrimitiveArray<T>::scrType;
	static PrimitiveArrayRegistrar<int8_t> int8ArrayRegistrar("int8");
	static PrimitiveArrayRegistrar<uint8_t> uint8ArrayRegistrar("uint8");
	static PrimitiveArrayRegistrar<int16_t> int16ArrayRegistrar("int16");
	static PrimitiveArrayRegistrar<uint16_t> uint16ArrayRegistrar("uint16");
	static PrimitiveArrayRegistrar<int32_t> int32ArrayRegistrar("int32");
	static PrimitiveArrayRegistrar<uint32_t> uint32ArrayRegistrar("uint32");
	static PrimitiveArrayRegistrar<int64_t> int64ArrayRegistrar("int64");
	static PrimitiveArrayRegistrar<uint64_t> uint64ArrayRegistrar("uint64");
	static PrimitiveArrayRegistrar<float> floatArrayRegistrar("float");
	static PrimitiveArrayRegistrar<double> doubleArrayRegistrar("double");

}
