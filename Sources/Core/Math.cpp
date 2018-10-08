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

#include <new>
#include <cstdlib>

#include "Math.h"
#include <Core/Debug.h>
#include <Core/ThreadLocalStorage.h>

namespace spades {
	/*
	 Vector3 Line3::Project(spades::Vector3 v,
	 bool supposeUnbounded) {

	 }

	 float Line3::GetDistanceTo(spades::Vector3 v,
	 bool supposeUnbounded){

	 }
	 */

	namespace {
		std::random_device r_device;
		std::mt19937_64 global_rng{r_device()};
		std::mutex global_rng_mutex;
	} // namespace

	/** Thread-local random number generator. */
	class LocalRNG {
	public:
		using result_type = std::uint64_t;
		
		LocalRNG() {
			auto sampleRandom = [] {
				std::lock_guard<std::mutex> lock{global_rng_mutex};
				return global_rng();
			};
			do {
				s[0] = sampleRandom();
			} while (s[0] == 0);
			do {
				s[1] = sampleRandom();
			} while (s[1] == 0);
		}
		
		result_type operator()() {
			uint64_t x = s[0];
			uint64_t y = s[1];
			s[0] = y;
			x ^= x << 23;                         // a
			s[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
			return s[1] + y;
		}
		
		float SampleFloat() { return std::uniform_real_distribution<float>{}(*this); }
		
		template <class T = int> inline T SampleInt(T a, T b) {
			return std::uniform_int_distribution<T>{a, b}(*this);
		}
		
		static constexpr result_type min() { return 0; }
		static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
		
	private:
		std::uint64_t s[2];
	};
	
	namespace {
		AutoDeletedThreadLocalStorage<LocalRNG> thread_local_rng;
		
		LocalRNG &GetThreadLocalRNG() {
			LocalRNG *rng = thread_local_rng.GetPointer();
			if (!rng) {
				thread_local_rng = rng = new LocalRNG();
			}
			return *rng;
		}
	} // namespace
	
	std::uint_fast64_t SampleRandom() { return GetThreadLocalRNG()(); }
	
	float SampleRandomFloat() {
		return std::uniform_real_distribution<float>{}(GetThreadLocalRNG());
	}
	
	template <class T> T SampleRandomInt(T a, T b) {
		return std::uniform_int_distribution<T>{a, b}(GetThreadLocalRNG());
	}

	// Note: `uniform_int_distribution` does not accept `char` nor `unsigned char`
	//       (N4659 29.6.1.1 [rand.req.genl])
	template short SampleRandomInt(short a, short b);
	template unsigned short SampleRandomInt(unsigned short a, unsigned short b);
	template int SampleRandomInt(int a, int b);
	template unsigned int SampleRandomInt(unsigned int a, unsigned int b);
	template long SampleRandomInt(long a, long b);
	template unsigned long SampleRandomInt(unsigned long a, unsigned long b);
	template long long SampleRandomInt(long long a, long long b);
	template unsigned long long SampleRandomInt(unsigned long long a, unsigned long long b);

	void Matrix4Multiply(const float a[16], const float b[16], float out[16]) {
		out[0] = b[0] * a[0] + b[1] * a[4] + b[2] * a[8] + b[3] * a[12];
		out[1] = b[0] * a[1] + b[1] * a[5] + b[2] * a[9] + b[3] * a[13];
		out[2] = b[0] * a[2] + b[1] * a[6] + b[2] * a[10] + b[3] * a[14];
		out[3] = b[0] * a[3] + b[1] * a[7] + b[2] * a[11] + b[3] * a[15];

		out[4] = b[4] * a[0] + b[5] * a[4] + b[6] * a[8] + b[7] * a[12];
		out[5] = b[4] * a[1] + b[5] * a[5] + b[6] * a[9] + b[7] * a[13];
		out[6] = b[4] * a[2] + b[5] * a[6] + b[6] * a[10] + b[7] * a[14];
		out[7] = b[4] * a[3] + b[5] * a[7] + b[6] * a[11] + b[7] * a[15];

		out[8] = b[8] * a[0] + b[9] * a[4] + b[10] * a[8] + b[11] * a[12];
		out[9] = b[8] * a[1] + b[9] * a[5] + b[10] * a[9] + b[11] * a[13];
		out[10] = b[8] * a[2] + b[9] * a[6] + b[10] * a[10] + b[11] * a[14];
		out[11] = b[8] * a[3] + b[9] * a[7] + b[10] * a[11] + b[11] * a[15];

		out[12] = b[12] * a[0] + b[13] * a[4] + b[14] * a[8] + b[15] * a[12];
		out[13] = b[12] * a[1] + b[13] * a[5] + b[14] * a[9] + b[15] * a[13];
		out[14] = b[12] * a[2] + b[13] * a[6] + b[14] * a[10] + b[15] * a[14];
		out[15] = b[12] * a[3] + b[13] * a[7] + b[14] * a[11] + b[15] * a[15];
	}

	Matrix4::Matrix4(float *elms) {
		for (int i = 0; i < 16; i++)
			m[i] = elms[i];
	}

	Matrix4::Matrix4(float m00, float m10, float m20, float m30, float m01, float m11, float m21,
	                 float m31, float m02, float m12, float m22, float m32, float m03, float m13,
	                 float m23, float m33) {
		m[0] = m00;
		m[1] = m10;
		m[2] = m20;
		m[3] = m30;
		m[4] = m01;
		m[5] = m11;
		m[6] = m21;
		m[7] = m31;
		m[8] = m02;
		m[9] = m12;
		m[10] = m22;
		m[11] = m32;
		m[12] = m03;
		m[13] = m13;
		m[14] = m23;
		m[15] = m33;
	}

	Matrix4 Matrix4::operator*(const spades::Matrix4 &other) const {
		Matrix4 out;
		Matrix4Multiply(m, other.m, out.m);
		return Matrix4(out);
	}

	Vector4 operator*(const Matrix4 &mat, const Vector4 &v) {
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

	Vector4 operator*(const Matrix4 &mat, const Vector3 &v) {
		return mat * MakeVector4(v.x, v.y, v.z, 1.f);
	}

	Vector3 Matrix4::GetOrigin() const { return MakeVector3(m[12], m[13], m[14]); }

	Vector3 Matrix4::GetAxis(int axis) const {
		switch (axis) {
			case 0: return MakeVector3(m[0], m[1], m[2]);
			case 1: return MakeVector3(m[4], m[5], m[6]);
			default: SPAssert(false);
			case 2: return MakeVector3(m[8], m[9], m[10]);
		}
	}

	Matrix4 Matrix4::FromAxis(spades::Vector3 a1, spades::Vector3 a2, spades::Vector3 a3,
	                          spades::Vector3 origin) {
		return Matrix4(a1.x, a1.y, a1.z, 0.f, a2.x, a2.y, a2.z, 0.f, a3.x, a3.y, a3.z, 0.f,
		               origin.x, origin.y, origin.z, 1.f);
	}

	Matrix4 Matrix4::Identity() { return Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1); }

	Matrix4 Matrix4::Translate(spades::Vector3 v) { return Translate(v.x, v.y, v.z); }

	Matrix4 Matrix4::Translate(float x, float y, float z) {
		return Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
	}

	Matrix4 Matrix4::Rotate(spades::Vector3 axis, float theta) {
		axis = axis.Normalize();

		float c = cosf(theta), s = sinf(theta);
		float ic = 1.f - c;
		float x = axis.x, y = axis.y, z = axis.z;

		return Matrix4(x * x * ic + c, x * y * ic + z * s, x * z * ic - y * s, 0,
		               x * y * ic - z * s, y * y * ic + c, y * z * ic + x * s, 0,
		               x * z * ic + y * s, y * z * ic - x * s, z * z * ic + c, 0, 0, 0, 0, 1);
	}
	Matrix4 Matrix4::Scale(float f) { return Scale(f, f, f); }
	Matrix4 Matrix4::Scale(spades::Vector3 v) { return Scale(v.x, v.y, v.z); }
	Matrix4 Matrix4::Scale(float x, float y, float z) {
		return Matrix4(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1);
	}
	Matrix4 Matrix4::Transposed() const {
		return Matrix4(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14],
		               m[3], m[7], m[11], m[15]);
	}

	static void inverseMatrix4(float *matrix) {
		// based on three.js's getInverse, which is in turn based on
		// http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm

		float n11 = matrix[0], n12 = matrix[4], n13 = matrix[8], n14 = matrix[12];
		float n21 = matrix[1], n22 = matrix[5], n23 = matrix[9], n24 = matrix[13];
		float n31 = matrix[2], n32 = matrix[6], n33 = matrix[10], n34 = matrix[14];
		float n41 = matrix[3], n42 = matrix[7], n43 = matrix[11], n44 = matrix[15];

		matrix[0] = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 -
		            n23 * n32 * n44 + n22 * n33 * n44;
		matrix[4] = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 +
		            n13 * n32 * n44 - n12 * n33 * n44;
		matrix[8] = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 -
		            n13 * n22 * n44 + n12 * n23 * n44;
		matrix[12] = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 +
		             n13 * n22 * n34 - n12 * n23 * n34;
		matrix[1] = n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 +
		            n23 * n31 * n44 - n21 * n33 * n44;
		matrix[5] = n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 -
		            n13 * n31 * n44 + n11 * n33 * n44;
		matrix[9] = n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 +
		            n13 * n21 * n44 - n11 * n23 * n44;
		matrix[13] = n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 -
		             n13 * n21 * n34 + n11 * n23 * n34;
		matrix[2] = n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 -
		            n22 * n31 * n44 + n21 * n32 * n44;
		matrix[6] = n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 +
		            n12 * n31 * n44 - n11 * n32 * n44;
		matrix[10] = n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 -
		             n12 * n21 * n44 + n11 * n22 * n44;
		matrix[14] = n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 +
		             n12 * n21 * n34 - n11 * n22 * n34;
		matrix[3] = n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 +
		            n22 * n31 * n43 - n21 * n32 * n43;
		matrix[7] = n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 -
		            n12 * n31 * n43 + n11 * n32 * n43;
		matrix[11] = n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 +
		             n12 * n21 * n43 - n11 * n22 * n43;
		matrix[15] = n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 -
		             n12 * n21 * n33 + n11 * n22 * n33;

		float det = n11 * matrix[0] + n21 * matrix[4] + n31 * matrix[8] + n41 * matrix[12];
		float idet = 1.0f / det;
		for (int i = 0; i < 16; ++i) {
			matrix[i] *= idet;
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
		return OBB3(
		  Matrix4(siz.x, 0, 0, 0, 0, siz.y, 0, 0, 0, 0, siz.z, 0, min.x, min.y, min.z, 1));
	}

	bool OBB3::RayCast(spades::Vector3 start, spades::Vector3 dir, spades::Vector3 *hitPos) {
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
		if (*this && start) {
			*hitPos = start;
			return true;
		}

		// x-plane hit test
		if (dotX != 0.f) {
			float startp = Vector3::Dot(start, normX);
			float endp = Vector3::Dot(end, normX);
			float boxp = Vector3::Dot(normX, normX);
			float hit; // 0=start, 1=end
			if (startp < endp) {
				hit = startp / (startp - endp);
			} else {
				hit = (boxp - startp) / (endp - startp);
			}
			if (hit >= 0.f) {
				*hitPos = start + dir * hit;

				float yd = Vector3::Dot(*hitPos, normY);
				float zd = Vector3::Dot(*hitPos, normZ);
				if (yd >= 0 && zd >= 0 && yd <= normY.GetPoweredLength() &&
				    zd <= normZ.GetPoweredLength()) {
					// hit x-plane
					*hitPos += origin;
					return true;
				}
			}
		}

		// y-plane hit test
		if (dotY != 0.f) {
			float startp = Vector3::Dot(start, normY);
			float endp = Vector3::Dot(end, normY);
			float boxp = Vector3::Dot(normY, normY);
			float hit; // 0=start, 1=end
			if (startp < endp) {
				hit = startp / (startp - endp);
			} else {
				hit = (boxp - startp) / (endp - startp);
			}
			if (hit >= 0.f) {
				*hitPos = start + dir * hit;

				float xd = Vector3::Dot(*hitPos, normX);
				float zd = Vector3::Dot(*hitPos, normZ);
				if (xd >= 0 && zd >= 0 && xd <= normX.GetPoweredLength() &&
				    zd <= normZ.GetPoweredLength()) {
					// hit y-plane
					*hitPos += origin;
					return true;
				}
			}
		}

		// z-plane hit test
		if (dotZ != 0.f) {
			float startp = Vector3::Dot(start, normZ);
			float endp = Vector3::Dot(end, normZ);
			float boxp = Vector3::Dot(normZ, normZ);
			float hit; // 0=start, 1=end
			if (startp < endp) {
				hit = startp / (startp - endp);
			} else {
				hit = (boxp - startp) / (endp - startp);
			}
			if (hit >= 0.f) {
				*hitPos = start + dir * hit;

				float xd = Vector3::Dot(*hitPos, normX);
				float yd = Vector3::Dot(*hitPos, normY);
				if (xd >= 0 && yd >= 0 && xd <= normX.GetPoweredLength() &&
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
		return r.x >= 0.f && r.y >= 0.f && r.z >= 0.f && r.x < 1.f && r.y < 1.f && r.z < 1.f;
	}

	float OBB3::GetDistanceTo(const spades::Vector3 &v) const {
		if (*this && v) {
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

	std::string Replace(const std::string &text, const std::string &before,
	                    const std::string &after) {
		std::string out;
		size_t pos = 0;
		while (pos < text.size()) {
			size_t newPos = text.find(before, pos);
			if (newPos == std::string::npos) {
				out.append(text, pos, text.size() - pos);
				break;
			}

			out.append(text, pos, newPos - pos);
			out += after;
			pos = newPos + before.size();
		}
		return out;
	}

	std::vector<std::string> Split(const std::string &str, const std::string &sep) {
		std::vector<std::string> strs;
		size_t pos = 0;
		while (pos < str.size()) {
			size_t newPos = str.find(sep, pos);
			if (newPos == std::string::npos) {
				strs.push_back(str.substr(pos));
				break;
			}
			strs.push_back(str.substr(pos, newPos - pos));
			pos = newPos + sep.size();
		}
		return strs;
	}

	std::vector<std::string> SplitIntoLines(const std::string &str) {
		std::vector<std::string> strs;
		size_t pos = 0;
		char newLineChar = 0;
		std::string buf;
		for (pos = 0; pos < str.size(); pos++) {
			if (str[pos] == 13 || str[pos] == 10) {
				if (newLineChar == 0)
					newLineChar = str[pos];
				if (str[pos] != newLineChar) {
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

	std::string EscapeControlCharacters(const std::string &str) {
		std::string out;
		out.reserve(str.size() * 2);
		for (size_t i = 0; i < str.size(); i++) {
			char c = str[i];
			if ((c >= 0 && c < 0x20 && c != 10 && c != 13 && c != 9)) {
				out += '<';
				switch (c) {
					case 0x00: out += "NUL"; break;
					case 0x01: out += "SOH"; break;
					case 0x02: out += "STX"; break;
					case 0x03: out += "ETX"; break;
					case 0x04: out += "EOT"; break;
					case 0x05: out += "ENQ"; break;
					case 0x06: out += "ACK"; break;
					case 0x07: out += "BEL"; break;
					case 0x08: out += "BS"; break;
					case 0x09: out += "TAB"; break;
					case 0x0a: out += "LF"; break;
					case 0x0b: out += "VT"; break;
					case 0x0c: out += "FF"; break;
					case 0x0d: out += "CR"; break;
					case 0x0e: out += "SO"; break;
					case 0x0f: out += "SI"; break;
					case 0x10: out += "DLE"; break;
					case 0x11: out += "DC1"; break;
					case 0x12: out += "DC2"; break;
					case 0x13: out += "DC3"; break;
					case 0x14: out += "DC4"; break;
					case 0x15: out += "NAK"; break;
					case 0x16: out += "SYN"; break;
					case 0x17: out += "ETB"; break;
					case 0x18: out += "CAN"; break;
					case 0x19: out += "EM"; break;
					case 0x1a: out += "SUB"; break;
					case 0x1b: out += "ESC"; break;
					case 0x1c: out += "FS"; break;
					case 0x1d: out += "GS"; break;
					case 0x1e: out += "RS"; break;
					case 0x1f: out += "US"; break;
				}
				out += '>';
			} else {
				out += c;
			}
		}
		return out;
	}

	std::string TrimSpaces(const std::string &str) {
		size_t pos = str.find_first_not_of(" \t\n\r");
		if (pos == std::string::npos)
			return std::string();
		size_t po2 = str.find_last_not_of(" \t\n\r");
		SPAssert(po2 != std::string::npos);
		SPAssert(po2 >= pos);
		return str.substr(pos, po2 - pos + 1);
	}

	bool PlaneCullTest(const Plane3 &plane, const AABB3 &box) {
		Vector3 testVertex;
		// find the vertex with the greatest distance value
		if (plane.n.x >= 0.f) {
			if (plane.n.y >= 0.f) {
				if (plane.n.z >= 0.f) {
					testVertex = box.max;
				} else {
					testVertex = MakeVector3(box.max.x, box.max.y, box.min.z);
				}
			} else {
				if (plane.n.z >= 0.f) {
					testVertex = MakeVector3(box.max.x, box.min.y, box.max.z);
				} else {
					testVertex = MakeVector3(box.max.x, box.min.y, box.min.z);
				}
			}
		} else {
			if (plane.n.y >= 0.f) {
				if (plane.n.z >= 0.f) {
					testVertex = MakeVector3(box.min.x, box.max.y, box.max.z);
				} else {
					testVertex = MakeVector3(box.min.x, box.max.y, box.min.z);
				}
			} else {
				if (plane.n.z >= 0.f) {
					testVertex = MakeVector3(box.min.x, box.min.y, box.max.z);
				} else {
					testVertex = box.min;
				}
			}
		}
		return plane.GetDistanceTo(testVertex) >= 0.f;
	}

	bool EqualsIgnoringCase(const std::string &a, const std::string &b) {
		if (a.size() != b.size())
			return false;
		if (&a == &b)
			return true;
		for (size_t siz = a.size(), i = 0; i < siz; i++) {
			char x = a[i], y = b[i];
			if (tolower(x) != tolower(y))
				return false;
		}
		return true;
	}

	uint32_t GetCodePointFromUTF8String(const std::string &str, size_t start, size_t *outNumBytes) {
		if (start >= str.size()) {
			if (outNumBytes)
				*outNumBytes = 0;
			return 0;
		}
		if ((str[start] & 0x80) == 0) {
			if (outNumBytes)
				*outNumBytes = 1;
			return (uint32_t)str[start];
		}

		// WARNING: start <= str.size() - 2 is bad (this leads to security holes)
		uint32_t ret;
		if ((str[start] & 0xe0) == 0xc0 && start + 2 <= str.size()) {
			if (outNumBytes)
				*outNumBytes = 2;
			ret = (str[start] & 0x1f) << 6;
			ret |= (str[start + 1] & 0x3f);
			return ret;
		}
		if ((str[start] & 0xf0) == 0xe0 && start + 3 <= str.size()) {
			if (outNumBytes)
				*outNumBytes = 3;
			ret = (str[start] & 0xf) << 12;
			ret |= (str[start + 1] & 0x3f) << 6;
			ret |= (str[start + 2] & 0x3f);
			return ret;
		}
		if ((str[start] & 0xf8) == 0xf0 && start + 4 <= str.size()) {
			if (outNumBytes)
				*outNumBytes = 4;
			ret = (str[start] & 0x7) << 18;
			ret |= (str[start + 1] & 0x3f) << 12;
			ret |= (str[start + 2] & 0x3f) << 6;
			ret |= (str[start + 3] & 0x3f);
			return ret;
		}
		if ((str[start] & 0xfc) == 0xf8 && start + 5 <= str.size()) {
			if (outNumBytes)
				*outNumBytes = 5;
			ret = (str[start] & 0x3) << 24;
			ret |= (str[start + 1] & 0x3f) << 18;
			ret |= (str[start + 2] & 0x3f) << 12;
			ret |= (str[start + 3] & 0x3f) << 6;
			ret |= (str[start + 4] & 0x3f);
			return ret;
		}
		if ((str[start] & 0xfe) == 0xfc && start + 6 <= str.size()) {
			if (outNumBytes)
				*outNumBytes = 6;
			ret = (str[start] & 0x1) << 30;
			ret |= (str[start + 1] & 0x3f) << 24;
			ret |= (str[start + 2] & 0x3f) << 18;
			ret |= (str[start + 3] & 0x3f) << 12;
			ret |= (str[start + 4] & 0x3f) << 6;
			ret |= (str[start + 5] & 0x3f);
			return ret;
		}

		// invalid UTF8
		if (outNumBytes)
			*outNumBytes = 1;
		return (uint32_t)str[start];
	}

	float SmoothStep(float v) { return v * v * (3.f - 2.f * v); }

	float Mix(float a, float b, float frac) { return a + (b - a) * frac; }
	Vector2 Mix(const Vector2 &a, const Vector2 &b, float frac) { return a + (b - a) * frac; }
	Vector3 Mix(const Vector3 &a, const Vector3 &b, float frac) { return a + (b - a) * frac; }
}
