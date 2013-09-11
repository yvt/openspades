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

#include "Math.h"
#include <stdlib.h>
#include "ScriptManager.h"
#include <new>

namespace spades {
	/*
	Vector3 Line3::Project(spades::Vector3 v,
						   bool supposeUnbounded) {
		
	}
	
	float Line3::GetDistanceTo(spades::Vector3 v,
							   bool supposeUnbounded){
		
	}
	*/
	void Matrix4Multiply(const float a[16], const float b[16], float out[16]) {
		out[ 0] = b[ 0]*a[ 0] + b[ 1]*a[ 4] + b[ 2]*a[ 8] + b[ 3]*a[12];
        out[ 1] = b[ 0]*a[ 1] + b[ 1]*a[ 5] + b[ 2]*a[ 9] + b[ 3]*a[13];
		out[ 2] = b[ 0]*a[ 2] + b[ 1]*a[ 6] + b[ 2]*a[10] + b[ 3]*a[14];
		out[ 3] = b[ 0]*a[ 3] + b[ 1]*a[ 7] + b[ 2]*a[11] + b[ 3]*a[15];
		
		out[ 4] = b[ 4]*a[ 0] + b[ 5]*a[ 4] + b[ 6]*a[ 8] + b[ 7]*a[12];
		out[ 5] = b[ 4]*a[ 1] + b[ 5]*a[ 5] + b[ 6]*a[ 9] + b[ 7]*a[13];
		out[ 6] = b[ 4]*a[ 2] + b[ 5]*a[ 6] + b[ 6]*a[10] + b[ 7]*a[14];
		out[ 7] = b[ 4]*a[ 3] + b[ 5]*a[ 7] + b[ 6]*a[11] + b[ 7]*a[15];
		
		out[ 8] = b[ 8]*a[ 0] + b[ 9]*a[ 4] + b[10]*a[ 8] + b[11]*a[12];
		out[ 9] = b[ 8]*a[ 1] + b[ 9]*a[ 5] + b[10]*a[ 9] + b[11]*a[13];
		out[10] = b[ 8]*a[ 2] + b[ 9]*a[ 6] + b[10]*a[10] + b[11]*a[14];
		out[11] = b[ 8]*a[ 3] + b[ 9]*a[ 7] + b[10]*a[11] + b[11]*a[15];
		
		out[12] = b[12]*a[ 0] + b[13]*a[ 4] + b[14]*a[ 8] + b[15]*a[12];
		out[13] = b[12]*a[ 1] + b[13]*a[ 5] + b[14]*a[ 9] + b[15]*a[13];
		out[14] = b[12]*a[ 2] + b[13]*a[ 6] + b[14]*a[10] + b[15]*a[14];
		out[15] = b[12]*a[ 3] + b[13]*a[ 7] + b[14]*a[11] + b[15]*a[15];
	}
	
	Matrix4::Matrix4(float *elms) {
		for(int i = 0; i < 16; i++)
			m[i] = elms[i];
	}
	
	Matrix4::Matrix4(float m00, float m10, float m20, float m30,
					 float m01, float m11, float m21, float m31,
					 float m02, float m12, float m22, float m32,
					 float m03, float m13, float m23, float m33){
		m[0] = m00; m[1] = m10; m[2] = m20; m[3] = m30;
		m[4] = m01; m[5] = m11; m[6] = m21; m[7] = m31;
		m[8] = m02; m[9] = m12; m[10] = m22; m[11] = m32;
		m[12] =m03; m[13] =m13; m[14]= m23;m[15] = m33;
	}
	
	Matrix4 Matrix4::operator*(const spades::Matrix4 &other) const {
		Matrix4 out;
		Matrix4Multiply(m, other.m, out.m);
		return Matrix4(out);
	}
	
	Vector4 operator *(const Matrix4& mat, const Vector4& v){
		float x, y, z, w;
		x = mat.m[0] * v.x;
		y = mat.m[1] * v.x;
		z = mat.m[2] * v.x;
		w = mat.m[3] * v.x;
		x += mat.m[4] * v.y;
		y += mat.m[5] * v.y;
		z += mat.m[6] * v.y;
		w += mat.m[7] * v.y;
		x += mat.m[8] * v.z;
		y += mat.m[9] * v.z;
		z += mat.m[10] * v.z;
		w += mat.m[11] * v.z;
		x += mat.m[12] * v.w;
		y += mat.m[13] * v.w;
		z += mat.m[14] * v.w;
		w += mat.m[15] * v.w;
		return MakeVector4(x, y, z, w);
	}
	
	Vector4 operator *(const Matrix4& mat, const Vector3& v){
		return mat * MakeVector4(v.x, v.y, v.z, 1.f);
	}
	
	Vector3 Matrix4::GetOrigin() const {
		return MakeVector3(m[12], m[13], m[14]);
	}
	
	Vector3 Matrix4::GetAxis(int axis) const {
		switch(axis){
			case 0:
				return MakeVector3(m[0], m[1], m[2]);
			case 1:
				return MakeVector3(m[4], m[5], m[6]);
			case 2:
				return MakeVector3(m[8], m[9], m[10]);
			default:
				SPAssert(false);
		}
	}
	
	Matrix4 Matrix4::FromAxis(spades::Vector3 a1,
							  spades::Vector3 a2,
							  spades::Vector3 a3,
							  spades::Vector3 origin) {
		return Matrix4(a1.x,a1.y,a1.z,0.f,
					   a2.x,a2.y,a2.z,0.f,
					   a3.x,a3.y,a3.z,0.f,
					   origin.x,origin.y,origin.z,1.f);
	}
	
	
	Matrix4 Matrix4::Identity() {
		return Matrix4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
	}
	
	Matrix4 Matrix4::Translate(spades::Vector3 v){
		return Translate(v.x, v.y, v.z);
	}
	
	Matrix4 Matrix4::Translate(float x, float y, float z){
		return Matrix4(1,0,0,0, 0,1,0,0, 0,0,1,0, x,y,z,1);
	}
	
	Matrix4 Matrix4::Rotate(spades::Vector3 axis, float theta){
		axis = axis.Normalize();
		
		float c = cosf(theta), s = sinf(theta);
		float ic = 1.f - c;
		float x = axis.x, y = axis.y, z = axis.z;
		
		return Matrix4(x*x*ic + c, x*y*ic + z * s, x*z*ic - y * s, 0,
					   x*y*ic - z * s, y*y*ic + c, y*z*ic + x * s, 0,
					   x*z*ic + y * s, y*z*ic - x * s, z*z*ic + c, 0,
					   0, 0, 0, 1);
	}
	Matrix4 Matrix4::Scale(float f){
		return Scale(f,f,f);
	}
	Matrix4 Matrix4::Scale(spades::Vector3 v){
		return Scale(v.x,v.y,v.z);
	}
	Matrix4 Matrix4::Scale(float x, float y, float z) {
		return Matrix4(x,0,0,0,0,y,0,0,0,0,z,0,0,0,0,1);
	}
	Matrix4 Matrix4::Transposed() const {
		return Matrix4(m[0], m[4], m[8], m[12],
					   m[1], m[5], m[9], m[13],
					   m[2], m[6], m[10],m[14],
					   m[3], m[7], m[11],m[15]);
	}
#define MADDR(x, y)		((x)+((y)<<2))
	// from BloodyKart
	static void inverseMatrix4(float *matrix){
		float det=1.f;
		float t, u;
		int k, j, i;
		for(k=0;k<4;k++){
			t=matrix[MADDR(k, k)];
			det*=t;
			for(i=0;i<4;i++)
				matrix[MADDR(k, i)]/=t;
			matrix[MADDR(k, k)]=1.f/t;
			for(j=0;j<4;j++){
				if(j==k)
					continue;
				u=matrix[MADDR(j, k)];
				for(i=0;i<4;i++)
					if(i!=k)
						matrix[MADDR(j, i)]-=matrix[MADDR(k, i)]*u;
					else
						matrix[MADDR(j, i)]=-u/t;
			}
		}
		
	}
	
	Matrix4 Matrix4::Inversed() const {
		Matrix4 mm = *this;
		inverseMatrix4(mm.m);
		return mm;
	}
	
	Matrix4 Matrix4::InversedFast() const {
		Matrix4 out = Matrix4::Identity();
		Vector3 axis1 = GetAxis(0);
		Vector3 axis2 = GetAxis(1);
		Vector3 axis3 = GetAxis(2);
		Vector3 norm1 = axis1 / axis1.GetPoweredLength();
		Vector3 norm2 = axis2 / axis2.GetPoweredLength();
		Vector3 norm3 = axis3 / axis3.GetPoweredLength();
		out.m[0] = norm1.x;
		out.m[1] = norm2.x;
		out.m[2] = norm3.x;
		out.m[4] = norm1.y;
		out.m[5] = norm2.y;
		out.m[6] = norm3.y;
		out.m[8] = norm1.z;
		out.m[9] = norm2.z;
		out.m[10] = norm3.z;
		
		Vector3 s = (out * GetOrigin()).GetXYZ();
		out.m[12] = -s.x;
		out.m[13] = -s.y;
		out.m[14] = -s.z;
		
		return out;
	}
	
	
	
	AABB3::operator OBB3() const {
		Vector3 siz = max - min;
		return OBB3(Matrix4(siz.x, 0, 0, 0,
							0, siz.y, 0, 0,
							0, 0, siz.z, 0,
							min.x, min.y, min.z, 1));
	}
	
	bool OBB3::RayCast(spades::Vector3 start,
					   spades::Vector3 dir,
					   spades::Vector3 *hitPos) {
		Vector3 normX = {m.m[0], m.m[1], m.m[2]};
		Vector3 normY = {m.m[4], m.m[5], m.m[6]};
		Vector3 normZ = {m.m[8], m.m[9], m.m[10]};
		
		// subtract offset
		Vector3 origin = {m.m[12], m.m[13], m.m[14]};
		start -= origin;
		
		Vector3 end = start + dir;
	
		float dotX = Vector3::Dot(dir, normX);
		float dotY = Vector3::Dot(dir, normY);
		float dotZ = Vector3::Dot(dir, normZ);
		
		// inside?
		if(*this && start){
			*hitPos = start;
			return true;
		}
		
		// x-plane hit test
		if(dotX != 0.f){
			float startp = Vector3::Dot(start, normX);
			float endp = Vector3::Dot(end, normX);
			float boxp = Vector3::Dot(normX, normX);
			float hit; // 0=start, 1=end
			if(startp < endp){
				hit = startp / (startp - endp);
			}else{
				hit = (boxp - startp) / (endp - startp);
			}
			if(hit >= 0.f){
				*hitPos = start + dir * hit;
				
				float yd = Vector3::Dot(*hitPos, normY);
				float zd = Vector3::Dot(*hitPos, normZ);
				if(yd >= 0 && zd >= 0 &&
				   yd <= normY.GetPoweredLength() &&
				   zd <= normZ.GetPoweredLength()) {
					// hit x-plane
					*hitPos += origin;
					return true;
				}
			}
		}
		
		// y-plane hit test
		if(dotY != 0.f){
			float startp = Vector3::Dot(start, normY);
			float endp = Vector3::Dot(end, normY);
			float boxp = Vector3::Dot(normY, normY);
			float hit; // 0=start, 1=end
			if(startp < endp){
				hit = startp / (startp - endp);
			}else{
				hit = (boxp - startp) / (endp - startp);
			}
			if(hit >= 0.f){
				*hitPos = start + dir * hit;
				
				float xd = Vector3::Dot(*hitPos, normX);
				float zd = Vector3::Dot(*hitPos, normZ);
				if(xd >= 0 && zd >= 0 &&
				   xd <= normX.GetPoweredLength() &&
				   zd <= normZ.GetPoweredLength()) {
					// hit y-plane
					*hitPos += origin;
					return true;
				}
			}
		}
		
		// z-plane hit test
		if(dotZ != 0.f){
			float startp = Vector3::Dot(start, normZ);
			float endp = Vector3::Dot(end, normZ);
			float boxp = Vector3::Dot(normZ, normZ);
			float hit; // 0=start, 1=end
			if(startp < endp){
				hit = startp / (startp - endp);
			}else{
				hit = (boxp - startp) / (endp - startp);
			}
			if(hit >= 0.f){
				*hitPos = start + dir * hit;
				
				float xd = Vector3::Dot(*hitPos, normX);
				float yd = Vector3::Dot(*hitPos, normY);
				if(xd >= 0 && yd >= 0 &&
				   xd <= normX.GetPoweredLength() &&
				   yd <= normY.GetPoweredLength()) {
					// hit z-plane
					*hitPos += origin;
					return true;
				}
			}
		}
		
		return false;
	}
	
	bool OBB3::operator&&(const spades::Vector3 &v) const {
		Matrix4 rmat = m.InversedFast();
		Vector3 r = (rmat * v).GetXYZ();
		return r.x >= 0.f && r.y >= 0.f && r.z >= 0.f &&
		r.x < 1.f && r.y < 1.f && r.z < 1.f;
	}
	
	float OBB3::GetDistanceTo(const spades::Vector3& v) const {
		if(*this && v){
			return 0.f;
		}
		
		Matrix4 rmat = m.InversedFast();
		Vector3 r = (rmat * v).GetXYZ();
		Vector3 pt = r;
		
		// find nearest point
		r.x = std::max(r.x, 0.f);
		r.y = std::max(r.y, 0.f);
		r.z = std::max(r.z, 0.f);
		r.x = std::min(r.x, 1.f);
		r.y = std::min(r.y, 1.f);
		r.z = std::min(r.z, 1.f);
		
		// compute difference in local space
		pt -= r;
		
		// scale to global space
		float len;
		len = pt.x * pt.x * m.GetAxis(0).GetPoweredLength();
		len += pt.y * pt.y * m.GetAxis(1).GetPoweredLength();
		len += pt.z * pt.z * m.GetAxis(2).GetPoweredLength();
		
		return len;
	}
	
	AABB3 OBB3::GetBoundingAABB() const {
		Vector3 orig = m.GetOrigin();
		Vector3 axis1 = m.GetAxis(0);
		Vector3 axis2 = m.GetAxis(1);
		Vector3 axis3 = m.GetAxis(2);
		AABB3 ab(orig.x, orig.y, orig.z, 0, 0, 0);
		ab += orig + axis1;
		ab += orig + axis2;
		ab += orig + axis1 + axis2;
		ab += orig + axis3;
		ab += orig + axis3 + axis1;
		ab += orig + axis3 + axis2;
		ab += orig + axis3 + axis1 + axis2;
		return ab;
	}
	
	std::string Replace(const std::string& text,
						const std::string& before,
						const std::string& after) {
		std::string out;
		size_t pos = 0;
		while(pos < text.size()) {
			size_t newPos = text.find(before, pos);
			if(newPos == std::string::npos) {
				out.append(text, pos, text.size() - pos);
				break;
			}
			
			out.append(text, pos, newPos - pos);
			out += after;
			pos = newPos + before.size();
		}
		return out;
	}
	
	std::vector<std::string> Split(const std::string& str, const std::string& sep){
		std::vector<std::string> strs;
		size_t pos = 0;
		while(pos < str.size()){
			size_t newPos = str.find(sep, pos);
			if(newPos == std::string::npos) {
				strs.push_back(str.substr(pos));
				break;
			}
			strs.push_back(str.substr(pos, newPos - pos));
			pos = newPos + sep.size();
		}
		return strs;
	}
	
	std::vector<std::string> SplitIntoLines(const std::string& str){
		std::vector<std::string> strs;
		size_t pos = 0;
		char newLineChar = 0;
		std::string buf;
		for(pos = 0; pos < str.size(); pos++){
			if(str[pos] == 13 || str[pos] == 10){
				if(newLineChar == 0)
					newLineChar = str[pos];
				if(str[pos] != newLineChar){
					continue;
				}
				strs.push_back(buf);
				buf.clear();
				continue;
			}
			buf += str[pos];
		}
		strs.push_back(buf);
		
		return strs;
	}
	
	std::string TrimSpaces(const std::string& str){
		size_t pos = str.find_first_not_of(" \t\n\r");
		if(pos == std::string::npos)
			return std::string();
		size_t po2 = str.find_last_not_of(" \t\n\r");
		SPAssert(po2 != std::string::npos);
		SPAssert(po2 >= pos);
		return str.substr(pos, po2 - pos + 1);
	}
	
	bool PlaneCullTest(const Plane3& plane, const AABB3& box){
		Vector3 testVertex;
		// find the vertex with the greatest distance value
		if(plane.n.x >= 0.f){
			if(plane.n.y >= 0.f){
				if(plane.n.z >= 0.f){
					testVertex = box.max;
				}else{
					testVertex = MakeVector3(box.max.x, box.max.y, box.min.z);
				}
			}else{
				if(plane.n.z >= 0.f){
					testVertex = MakeVector3(box.max.x, box.min.y, box.max.z);
				}else{
					testVertex = MakeVector3(box.max.x, box.min.y, box.min.z);
				}
			}
		}else{
			if(plane.n.y >= 0.f){
				if(plane.n.z >= 0.f){
					testVertex = MakeVector3(box.min.x, box.max.y, box.max.z);
				}else{
					testVertex = MakeVector3(box.min.x, box.max.y, box.min.z);
				}
			}else{
				if(plane.n.z >= 0.f){
					testVertex = MakeVector3(box.min.x, box.min.y, box.max.z);
				}else{
					testVertex = box.min;
				}
			}
		}
		return plane.GetDistanceTo(testVertex) >= 0.f;
	}
	
	bool EqualsIgnoringCase(const std::string& a,
							const std::string& b) {
		if(a.size() != b.size())
			return false;
		if(&a == &b)
			return true;
		for(size_t siz = a.size(), i = 0; i < siz; i++) {
			char x = a[i], y = b[i];
			if(tolower(x) != tolower(y))
				return false;
		}
		return true;
	}
	
	float GetRandom() {
		return (float)rand() / (float)RAND_MAX;
	}
	float SmoothStep(float v){
		return v * v * (3.f - 2.f * v);
	}
	
	float Mix(float a, float b, float frac) {
		return a + (b - a) * frac;
	}
	Vector2 Mix(Vector2 a, Vector2 b, float frac) {
		return a + (b - a) * frac;
	}
	Vector3 Mix(Vector3 a, Vector3 b, float frac) {
		return a + (b - a) * frac;
	}
	
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
