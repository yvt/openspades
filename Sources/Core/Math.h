/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include <vector>
#include <math.h>
#include <string>
#include <stdint.h>		// uint32_t --> msvc
#include <algorithm>	// std::max / std::min

#ifdef _MSC_VER
#define isnan _isnan
static inline float roundf(float x) { return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f); }
static inline long lround(double num) { return (long)(num > 0 ? num + 0.5 : ceil(num - 0.5)); }
#endif

namespace spades {
	
#pragma mark - Integer Vector
	
	class IntVector3 {
	public:
		int x, y, z;
		
		IntVector3() = default;
		IntVector3(const IntVector3&) = default;
		IntVector3(int x, int y, int z):
		x(x), y(y), z(z) { }
		
		static IntVector3 Make(int x, int y, int z) {
			IntVector3 v = {x, y, z};
			return v;
		}
		
		static int Dot(const IntVector3& a,
					   const IntVector3& b) {
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}
		
		IntVector3 operator + (const IntVector3& v) const {
			return Make(x + v.x, y + v.y, z + v.z);
		}
		IntVector3 operator - (const IntVector3& v) const {
			return Make(x - v.x, y - v.y, z - v.z);
		}
		IntVector3 operator * (const IntVector3& v) const {
			return Make(x * v.x, y * v.y, z * v.z);
		}
		IntVector3 operator / (const IntVector3& v) const {
			return Make(x / v.x, y / v.y, z / v.z);
		}
		
		IntVector3 operator + (int v) const {
			return Make(x + v, y + v, z + v);
		}
		IntVector3 operator - (int v) const {
			return Make(x - v, y - v, z - v);
		}
		IntVector3 operator * (int v) const {
			return Make(x * v, y * v, z * v);
		}
		IntVector3 operator / (int v) const {
			return Make(x / v, y / v, z / v);
		}
		
		IntVector3 operator - () const {
			return Make(-x, -y, -z);
		}
		
		bool operator == (const IntVector3& v) const {
			return x == v.x && y == v.y && z == v.z;
		}
		
		int GetManhattanLength() const {
			return std::max(x, -x) + std::max(y, -y)
			 + std::max(z, -z);
		}
		
		int GetChebyshevLength() const {
			return std::max(std::max(x, -x),
							std::max(std::max(y, -y),
									 std::max(z, -z)));
		}
		
		IntVector3 &operator +=(const IntVector3& v) {
			x += v.x; y += v.y; z += v.z; return *this;
		}
		
		IntVector3 &operator -=(const IntVector3& v) {
			x -= v.x; y -= v.y; z -= v.z; return *this;
		}
		
		IntVector3 &operator *=(const IntVector3& v) {
			x *= v.x; y *= v.y; z *= v.z; return *this;
		}
		
		IntVector3 &operator /=(const IntVector3& v) {
			x /= v.x; y /= v.y; z /= v.z; return *this;
		}
		
		IntVector3& operator +=(int v) {
			x += v; y += v; z += v;
			return *this;
		}
		
		IntVector3& operator -=(int v) {
			x -= v; y -= v; z -= v;
			return *this;
		}
		
		IntVector3& operator *=(int v) {
			x *= v; y *= v; z *= v;
			return *this;
		}
		
		IntVector3& operator /=(int v) {
			x /= v; y /= v; z /= v;
			return *this;
		}
	};
	
#pragma mark - Real Vector
	
	class Vector2 {
	public:
		float x, y;
		
		Vector2() = default;
		Vector2(const Vector2&) = default;
		Vector2(float x, float y):
		x(x), y(y) {}
		
		static Vector2 Make(float x, float y) {
			Vector2 v = {x, y};
			return v;
		}
		
		Vector2 operator + (const Vector2& v) const {
			return Make(x + v.x, y + v.y);
		}
		Vector2 operator - (const Vector2& v) const {
			return Make(x - v.x, y - v.y);
		}
		Vector2 operator * (const Vector2& v) const {
			return Make(x * v.x, y * v.y);
		}
		Vector2 operator / (const Vector2& v) const {
			return Make(x / v.x, y / v.y);
		}
		
		Vector2 operator + (float v) const {
			return Make(x + v, y + v);
		}
		Vector2 operator - (float v) const {
			return Make(x - v, y - v);
		}
		Vector2 operator * (float v) const {
			return Make(x * v, y * v);
		}
		Vector2 operator / (float v) const {
			return Make(x / v, y / v);
		}
		
		Vector2 operator - () const {
			return Make(-x, -y);
		}
		bool operator == (const Vector2& v) const {
			return x == v.x && y == v.y;
		}
		
		Vector2& operator +=(const Vector2& v) {
			x += v.x; y += v.y;  return *this;
		}
		
		Vector2& operator -=(const Vector2& v) {
			x -= v.x; y -= v.y; return *this;
		}
		
		Vector2& operator *=(const Vector2& v) {
			x *= v.x; y *= v.y; return *this;
		}
		
		Vector2& operator /=(const Vector2& v) {
			x /= v.x; y /= v.y; return *this;
		}
		
		Vector2& operator +=(float v) {
			x += v; y += v; return *this;
		}
		
		Vector2& operator -=(float v) {
			x -= v; y -= v;  return *this;
		}
		
		Vector2& operator *=(float v) {
			x *= v; y *= v;  return *this;
		}
		
		Vector2& operator /=(float v) {
			x /= v; y /= v;  return *this;
		}
		
		static float Dot(const Vector2& a,
						 const Vector2& b) {
			return a.x * b.x + a.y * b.y;
		}
		
		float GetPoweredLength() const {
			return x * x + y * y;
		}
		
		float GetLength() const {
			return sqrtf(GetPoweredLength());
		}
		
		float GetManhattanLength() const {
			return fabsf(x) + fabsf(y);
		}
		
		float GetChebyshevLength() const {
			return std::max(fabsf(x), fabsf(y));
		}
		
		Vector2 Normalize() const {
			float scale = 1.f / GetLength();
			return Make(x * scale, y * scale);
		}
	};
	
	class Vector3 {
	public:
		float x, y, z;
		
		Vector3() = default;
		Vector3(const Vector3&) = default;
		Vector3(float x, float y, float z):
		x(x), y(y), z(z) {}
		
		static Vector3 Make(float x, float y, float z) {
			Vector3 v = {x, y, z};
			return v;
		}
		
		Vector3 operator + (const Vector3& v) const {
			return Make(x + v.x, y + v.y, z + v.z);
		}
		Vector3 operator - (const Vector3& v) const {
			return Make(x - v.x, y - v.y, z - v.z);
		}
		Vector3 operator * (const Vector3& v) const {
			return Make(x * v.x, y * v.y, z * v.z);
		}
		Vector3 operator / (const Vector3& v) const {
			return Make(x / v.x, y / v.y, z / v.z);
		}
		
		Vector3 operator + (float v) const {
			return Make(x + v, y + v, z + v);
		}
		Vector3 operator - (float v) const {
			return Make(x - v, y - v, z - v);
		}
		Vector3 operator * (float v) const {
			return Make(x * v, y * v, z * v);
		}
		Vector3 operator / (float v) const {
			return Make(x / v, y / v, z / v);
		}
		
		Vector3 operator - () const {
			return Make(-x, -y, -z);
		}
		bool operator == (const Vector3& v) const {
			return x == v.x && y == v.y && z == v.z;
		}
		
		Vector3& operator +=(const Vector3& v) {
			x += v.x; y += v.y; z += v.z;
			return *this;
		}
		
		Vector3& operator -=(const Vector3& v) {
			x -= v.x; y -= v.y; z -= v.z;
			return *this;
		}
		
		Vector3& operator *=(const Vector3& v) {
			x *= v.x; y *= v.y; z *= v.z;
			return *this;
		}
		
		Vector3& operator /=(const Vector3& v) {
			x /= v.x; y /= v.y; z /= v.z;
			return *this;
		}
		
		Vector3& operator +=(float v) {
			x += v; y += v; z += v;
			return *this;
		}
		
		Vector3& operator -=(float v) {
			x -= v; y -= v; z -= v;
			return *this;
		}
		
		Vector3& operator *=(float v) {
			x *= v; y *= v; z *= v;
			return *this;
		}
		
		Vector3& operator /=(float v) {
			x /= v; y /= v; z /= v;
			return *this;
		}
		
		static float Dot(const Vector3& a,
						 const Vector3& b) {
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}
		
		static Vector3 Cross(const Vector3& a,
							 const Vector3& b) {
			return Make(a.y*b.z-a.z*b.y,
						a.z*b.x-a.x*b.z,
						a.x*b.y-a.y*b.x);
		}
		
		static Vector3 Normal(const Vector3& a,
							  const Vector3& b,
							  const Vector3& c) {
			return Cross(c - a, c - b).Normalize();
		}
		
		float GetPoweredLength() const {
			return x * x + y * y + z * z;
		}
		
		float GetLength() const {
			return sqrtf(GetPoweredLength());
		}
		
		float GetManhattanLength() const {
			return fabsf(x) + fabsf(y) + fabsf(z);
		}
		
		float GetChebyshevLength() const {
			return std::max(fabsf(x), std::max(fabsf(y), fabsf(z)));
		}
		
		Vector3 Normalize() const {
			float scale = GetLength();
			if(scale != 0.f) scale = 1.f / scale;
			return Make(x * scale, y * scale, z * scale);
		}
		
		IntVector3 Floor() const{
			return IntVector3::Make((long)floorf(x),
									(long)floorf(y),
									(long)floorf(z));
		}
	};
	
	class Vector4 {
	public:
		float x, y, z, w;
		
		Vector4() = default;
		Vector4(const Vector4&) = default;
		Vector4(float x, float y, float z, float w):
		x(x), y(y), z(z), w(w) {}
		
		static Vector4 Make(float x, float y, float z, float w) {
			Vector4 v = {x, y, z, w};
			return v;
		}
		
		Vector4 operator + (const Vector4& v) const {
			return Make(x + v.x, y + v.y, z + v.z, w + v.w);
		}
		Vector4 operator - (const Vector4& v) const {
			return Make(x - v.x, y - v.y, z - v.z, w - v.w);
		}
		Vector4 operator * (const Vector4& v) const {
			return Make(x * v.x, y * v.y, z * v.z, w * v.w);
		}
		Vector4 operator / (const Vector4& v) const {
			return Make(x / v.x, y / v.y, z / v.z, w / v.w);
		}
		
		Vector4 operator + (float v) const {
			return Make(x + v, y + v, z + v, w + v);
		}
		Vector4 operator - (float v) const {
			return Make(x - v, y - v, z - v, w - v);
		}
		Vector4 operator * (float v) const {
			return Make(x * v, y * v, z * v, w * v);
		}
		Vector4 operator / (float v) const {
			return Make(x / v, y / v, z / v, w / v);
		}
		
		Vector4 operator - () const {
			return Make(-x, -y, -z, -w);
		}
		bool operator == (const Vector4& v) const {
			return x == v.x && y == v.y && z == v.z &&
			w == v.w;
		}
		
		
		Vector4& operator +=(const Vector4& v) {
			x += v.x; y += v.y; z += v.z; w += v.w;
			return *this;
		}
		
		Vector4& operator -=(const Vector4& v) {
			x -= v.x; y -= v.y; z -= v.z; w -= v.w;
			return *this;
		}
		
		Vector4& operator *=(const Vector4& v) {
			x *= v.x; y *= v.y; z *= v.z; w *= v.w;
			return *this;
		}
		
		Vector4& operator /=(const Vector4& v) {
			x /= v.x; y /= v.y; z /= v.z; w /= v.w;
			return *this;
		}
		
		Vector4& operator +=(float v) {
			x += v; y += v; z += v; w += v;
			return *this;
		}
		
		Vector4& operator -=(float v) {
			x -= v; y -= v; z -= v; w -= v;
			return *this;
		}
		
		Vector4& operator *=(float v) {
			x *= v; y *= v; z *= v; w *= v;
			return *this;
		}
		
		Vector4& operator /=(float v) {
			x /= v; y /= v; z /= v; w /= v;
			return *this;
		}
		
		static float Dot(const Vector4& a,
						 const Vector4& b) {
			return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
		}
				
		float GetPoweredLength() const {
			return x * x + y * y + z * z + w * w;
		}
		
		float GetLength() const {
			return sqrtf(GetPoweredLength());
		}
		
		float GetManhattanLength() const {
			return fabsf(x) + fabsf(y) + fabsf(z) + fabsf(w);
		}
		
		float GetChebyshevLength() const {
			return std::max(std::max(fabsf(x), std::max(fabsf(y), fabsf(z))), fabsf(w));
		}
		
		Vector4 Normalize() const {
			float scale = 1.f / GetLength();
			return Make(x * scale, y * scale, z * scale, w * scale);
		}
		
		Vector3 GetXYZ() const {
			return Vector3::Make(x, y, z);
		}
	};
	
	static inline Vector2 MakeVector2(float x, float y) {
		return Vector2::Make(x, y);
	}
	static inline Vector3 MakeVector3(float x, float y, float z) {
		return Vector3::Make(x, y, z);
	}
	static inline Vector3 MakeVector3(IntVector3 v) {
		return Vector3::Make( static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
	}
	static inline Vector4 MakeVector4(float x, float y, float z, float w) {
		return Vector4::Make(x, y, z, w);
	}
	
#pragma mark - Line
	
	
	struct Line3 {
		Vector3 v1, v2;
		
		/** true if line if bounded at v1 */
		bool end1;
		
		/** true if line if bounded at v2 */
		bool end2;
		
		Line3(){}
		Line3(Vector3 a, Vector3 b,
					 bool e1, bool e2):
		v1(a), v2(b), end1(e1), end2(e2){}
		static Line3 MakeLineSegment(Vector3 a, Vector3 b){
			return Line3(a, b, true, true);
		}
		static Line3 MakeLine(Vector3 a, Vector3 b){
			return Line3(a, b, false, false);
		}
		
		float GetDistanceTo(Vector3, bool supposeUnbounded = false);
		Vector3 Project(Vector3, bool supposeUnbounded = false);

	};
	
#pragma mark - Plane
	
	struct Plane3 {
		Vector3 n;
		float w;
		
		Plane3() = default;
		Plane3(const Plane3&) = default;
		Plane3(float x, float y, float z, float ww):
		n(MakeVector3(x, y, z)), w(ww) { }
		Plane3(const Vector3& v1,
			   const Vector3& v2,
			   const Vector3& v3) {
			n = Vector3::Normal(v1, v2, v3);
			w = -Vector3::Dot(n, v1);
		}
		
		static Plane3 PlaneWithPointOnPlane(Vector3 point, Vector3 normal) {
			return Plane3(normal.x, normal.y, normal.z,
						  -Vector3::Dot(point, normal));
		}
		
		Plane3 Flipped() const {
			return Plane3(-n.x, -n.y, -n.z, -w);
		}
		
		float GetDistanceTo(Vector3 v) const {
			return Vector3::Dot(n, v) + w;
		}
		
		Vector3 Project(Vector3 v) const {
			return v - n * GetDistanceTo(v);
		}
		
		Plane3 TowardTo(Vector3 v) const {
			if(GetDistanceTo(v) < 0.f)
				return -*this;
			else
				return *this;
		}
		
		Plane3 operator - () const {
			return Plane3(-n.x, -n.y, -n.z, w);
		}
	};
	
	struct Plane2 {
		Vector2 n;
		float w;
		
		Plane2() = default;
		Plane2(const Plane2&) = default;
		Plane2(float x, float y, float ww):
		n(Vector2::Make(x, y)), w(ww) { }
		Plane2(const Vector2& v1,
			  const Vector2& v2) {
			n = v2 - v1;
			n = Vector2::Make(n.y, -n.x);
			w = -Vector2::Dot(n, v1);
		}
		
		float GetDistanceTo(Vector2 v) const {
			return Vector2::Dot(n, v) + w;
		}
		
		Vector2 Project(Vector2 v) const {
			return v - n * GetDistanceTo(v);
		}
		
		Plane2 TowardTo(Vector2 v) const {
			if(GetDistanceTo(v) < 0.f)
				return -*this;
			else
				return *this;
		}

		Plane2 operator - () const {
			return Plane2(-n.x, -n.y, w);
		}
	};
		
#pragma mark - Matrix
	
	void Matrix4Multiply(const float a[16], const float b[16], float out[16]);
	
	struct Matrix4 {
		// column major matrix
		float m[16];
		Matrix4() = default;
		Matrix4(const Matrix4&) = default;
		explicit Matrix4(float *elms);
		Matrix4(float m00, float m10, float m20, float m30,
				float m01, float m11, float m21, float m31,
				float m02, float m12, float m22, float m32,
				float m03, float m13, float m23, float m33);
		
		static Matrix4 Identity();
		static Matrix4 Translate(Vector3);
		static Matrix4 Translate(float,float,float);
		static Matrix4 Rotate(Vector3 axis, float theta);
		static Matrix4 Scale(float);
		static Matrix4 Scale(Vector3);
		static Matrix4 Scale(float,float,float);
		
		Matrix4 Transposed() const;
		Matrix4 Inversed() const;
		
		/** inverses perpendicular matrix */
		Matrix4 InversedFast() const;
		
		Vector3 GetAxis(int) const;
		Vector3 GetOrigin() const;
		
		static Matrix4 FromAxis(Vector3 a1, Vector3 a2, Vector3 a3,
								Vector3 origin);
		
		Matrix4 operator *(const Matrix4& other) const;
		Matrix4 &operator *= (const Matrix4& other) {
			*this = *this * other;
			return *this;
		}
	};
	
	Vector4 operator *(const Matrix4& mat, const Vector4& v);
	Vector4 operator *(const Matrix4& mat, const Vector3& v);
	//Vector4 operator *(const Vector4& v, const Matrix4& mat);
	
	
#pragma mark - Axis-Aligned Bounding Box
	
	class AABB2 {
	public:
		Vector2 min, max;
		
		AABB2(){}
		AABB2(Vector2 minVector,
			  Vector2 maxVector):
		min(minVector), max(maxVector){}
		
		AABB2(float minX, float minY,
			  float width, float height):
		min(Vector2::Make(minX, minY)),
		max(Vector2::Make(minX + width, minY + height)){}
		
		AABB2 Inflate(float amount) const {
			return AABB2(min - Vector2::Make(amount, amount),
						 max + Vector2::Make(amount, amount));
		}
		
		AABB2 Translated(float x, float y) const {
			return AABB2(min + Vector2::Make(x, y),
						 max + Vector2::Make(x, y));
		}
		AABB2 Translated(Vector2 v) const{
			return AABB2(min + v,
						 max + v);
		}
		
		float GetMinX() const { return min.x; }
		float GetMaxX() const { return max.x;}
		float GetMinY() const { return min.y; }
		float GetMaxY() const { return max.y; }
		float GetWidth() const { return GetMaxX() - GetMinX(); }
		float GetHeight() const { return GetMaxY() - GetMinY(); }
		
		bool operator &&(const Vector2& v) const {
			return v.x >= min.x && v.y >= min.y &&
			v.x < max.x && v.y < max.y;
		}
			
		bool operator &&(const AABB2& o) const {
			return min.x < o.max.x &&
			   min.y < o.max.y &&
			   max.x > o.min.x &&
			   max.y > o.min.y;
		}
	
		bool Contains(const Vector2& v) const {
			return *this && v;
		}
		bool Intersects(const AABB2& v) const {
			return *this && v;
		}
			
		void operator += (const Vector2& vec) {
			if(vec.x < min.x) min.x = vec.x;
			if(vec.y < min.y) min.y = vec.y;
			if(vec.x > max.x) max.x = vec.x;
			if(vec.y > max.y) max.y = vec.y;
		}
		
		void operator += (const AABB2& b) {
			if(b.min.x < min.x) min.x = b.min.x;
			if(b.min.y < min.y) min.y = b.min.y;
			if(b.max.x > max.x) max.x = b.max.x;
			if(b.max.y > max.y) max.y = b.max.y;
		}
		
	};
	
	class OBB3;
	
	class AABB3 {
	public:
		Vector3 min, max;
		
		AABB3(){}
		AABB3(Vector3 minVector,
			  Vector3 maxVector):
		min(minVector), max(maxVector){}
		
		AABB3(float minX, float minY, float minZ,
			  float width, float height, float depth):
		min(Vector3::Make(minX, minY, minZ)),
		max(Vector3::Make(minX + width, minY + height, minZ + depth)){}
		
		AABB3 Inflate(float amount) const {
			return AABB3(min - Vector3::Make(amount, amount, amount),
						 max + Vector3::Make(amount, amount, amount));
		}
		
		float GetMinX() const { return min.x; }
		float GetMaxX() const { return max.x;}
		float GetMinY() const { return min.y; }
		float GetMaxY() const { return max.y; }
		float GetMinZ() const { return min.z; }
		float GetMaxZ() const { return max.z; }
		float GetWidth() const { return GetMaxX() - GetMinX(); }
		float GetHeight() const { return GetMaxY() - GetMinY(); }
		float GetDepth() const { return GetMaxZ() - GetMinZ(); }
		
		operator OBB3 () const;

		bool operator &&(const Vector3& v) const {
			return v.x >= min.x && v.y >= min.y && v.z >= min.z &&
			v.x < max.x && v.y < max.y && v.z < max.z;
		}
		
		bool operator &&(const AABB3& o) const {
			return min.x < o.max.x &&
			min.y < o.max.y &&
			min.z < o.max.z &&
			max.x > o.min.x &&
			max.y > o.min.y &&
			max.z > o.min.z;
		}
		bool Contains(const Vector3& v) const {
			return *this && v;
		}
		bool Intersects(const AABB3& v) const {
			return *this && v;
		}
		bool Contains(const AABB3& v) const {
			return v.min.x >= min.x && v.min.y >= min.y && v.min.z >= min.z &&
			v.max.x <= max.x && v.max.y <= max.y && v.max.z <= max.z;
		}
		
		void operator += (const Vector3& vec) {
			if(vec.x < min.x) min.x = vec.x;
			if(vec.y < min.y) min.y = vec.y;
			if(vec.z < min.z) min.z = vec.z;
			if(vec.x > max.x) max.x = vec.x;
			if(vec.y > max.y) max.y = vec.y;
			if(vec.z > max.z) max.z = vec.z;
		}
		
		void operator += (const AABB3& b) {
			if(b.min.x < min.x) min.x = b.min.x;
			if(b.min.y < min.y) min.y = b.min.y;
			if(b.min.z < min.z) min.z = b.min.z;
			if(b.max.x > max.x) max.x = b.max.x;
			if(b.max.y > max.y) max.y = b.max.y;
			if(b.max.z > max.z) max.z = b.max.z;
		}
		
	};
	
	
	
#pragma mark - Oriented Bounding Box
	
	class OBB3{
	public:
		/** matrix M where M * [x, y, z] = (coord in box)
		 * (0 <= x, y, z, <= 1) */
		Matrix4 m;
		
		OBB3(): m(Matrix4::Identity()){}
		OBB3(const Matrix4& mm):m(mm){}
		
		OBB3 operator *(const Matrix4& mm){
			return OBB3(m * mm);
		}
		
		bool operator &&(const Vector3& v) const ;
		float GetDistanceTo(const Vector3&) const;
		bool RayCast(Vector3 start, Vector3 dir, Vector3 *hitPos);
		AABB3 GetBoundingAABB() const;
	};
	
	static inline OBB3 operator *(const Matrix4&m, const OBB3& b) {
		return OBB3(m * b.m);
	}
	
#pragma mark - Utilities

	template <typename T>
	static inline void FastErase(std::vector<T>&  vec, size_t index){
		SPAssert(index < vec.size());
		if(index < vec.size() - 1){
			vec[index] = vec[vec.size() - 1];
		}
		vec.resize(vec.size() - 1);
	}
			
	float Mix(float a, float b, float frac);
	Vector2 Mix(const Vector2& a, const Vector2& b, float frac);
	Vector3 Mix(const Vector3& a, const Vector3& b, float frac);
	
	/** @return true if any portion of the box is in the positive side of plane. */
	bool PlaneCullTest(const Plane3&, const AABB3&);
	
	std::string Replace(const std::string& text,
						const std::string& before,
						const std::string& after);
			
	bool EqualsIgnoringCase(const std::string&,
							const std::string&);
			
	std::vector<std::string> Split(const std::string&, const std::string& sep);

	std::vector<std::string> SplitIntoLines(const std::string&);
	std::string EscapeControlCharacters(const std::string& str);
	
	uint32_t GetCodePointFromUTF8String(const std::string&, size_t start = 0, size_t *outNumBytes = nullptr);
	template<typename Iterator>
	static Iterator CodePointToUTF8(Iterator output, uint32_t cp) {
		if(cp < 0x80) {
			*(output++) = static_cast<char>(cp);
		}else if(cp < 0x800) {
			*(output++) = static_cast<char>((cp>>6) | 0xc0);
			*(output++) = static_cast<char>((cp&0x3f) | 0x80);
		}else if(cp < 0x10000) {
			*(output++) = static_cast<char>((cp>>12) | 0xe0);
			*(output++) = static_cast<char>(((cp>>6)&0x3f) | 0x80);
			*(output++) = static_cast<char>((cp&0x3f) | 0x80);
		}else if(cp < 0x200000) {
			*(output++) = static_cast<char>((cp>>18) | 0xf0);
			*(output++) = static_cast<char>(((cp>>12)&0x3f) | 0x80);
			*(output++) = static_cast<char>(((cp>>6)&0x3f) | 0x80);
			*(output++) = static_cast<char>((cp&0x3f) | 0x80);
		}else if(cp < 0x4000000) {
			*(output++) = static_cast<char>((cp>>24) | 0xf8);
			*(output++) = static_cast<char>(((cp>>18)&0x3f) | 0x80);
			*(output++) = static_cast<char>(((cp>>12)&0x3f) | 0x80);
			*(output++) = static_cast<char>(((cp>>6)&0x3f) | 0x80);
			*(output++) = static_cast<char>((cp&0x3f) | 0x80);
		}else{
			*(output++) = static_cast<char>((cp>>30) | 0xfc);
			*(output++) = static_cast<char>(((cp>>24)&0x3f) | 0x80);
			*(output++) = static_cast<char>(((cp>>18)&0x3f) | 0x80);
			*(output++) = static_cast<char>(((cp>>12)&0x3f) | 0x80);
			*(output++) = static_cast<char>(((cp>>6)&0x3f) | 0x80);
			*(output++) = static_cast<char>((cp&0x3f) | 0x80);
		}
		return output;
	}

	std::string TrimSpaces(const std::string&);
	
	float GetRandom();
	float SmoothStep(float);
}
