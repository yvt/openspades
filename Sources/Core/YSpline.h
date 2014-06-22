//
// YSpline (MIT license) by yvt <i@yvt.jp>
//
// -------------
// Quaternion spline interpolation is based on http://qspline.sourceforge.net/qspline.pdf
// -------------
//

#include <functional>
#include <valarray>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <array>
#include <complex>

namespace ysp {
	
	namespace detail { namespace {
		
		/** solves M * x = y. M is defined as:
		 * <code>
		 *	b0	a0
		 *	c1	b1	a1
		 *		c2	b2	a2
		 *			c3	b3	a3
		 *				...
		 *					cm	bm	am
		 *						cn	bn
		 * where m = n - 1
		 * </code>
		 */
		template <class T>
		void solve_diag(std::valarray<T>& x,
						std::valarray<T>& a,
						std::valarray<T>& b,
						std::valarray<T>& c,
						std::valarray<T>& y) {
			auto N = x.size();
			assert(N == a.size());
			assert(N == b.size());
			assert(N == c.size());
			assert(N == y.size());
			
			// forward: eliminate c
			for (std::size_t i = 1; i < N; ++i) {
				auto m = c[i] / b[i - 1];
				b[i] -= a[i - 1] * m;
				y[i] -= y[i - 1] * m;
			}
			
			// backward: eliminate a
			x[N - 1] = y[N - 1] / b[N - 1];
			for (std::size_t i = N - 1; i > 0; --i) {
				x[i - 1] = (y[i - 1] - x[i] * a[i - 1]) / b[i - 1];
			}
		}
		
	} }
	
	template <class T, int N>
	class dense_vector {
		template <int I, class T_, int N_> friend class dense_vector_getter;
		T head;
		dense_vector<T, N - 1> tail;
	public:
		dense_vector() = default;
		dense_vector(const dense_vector&) = default;
		dense_vector(T head, const dense_vector<T, N - 1>& tail):
		head(head), tail(tail) { }
		template <class... Next>
		inline dense_vector(T head, Next... tail):
		head(head), tail(tail...) { }
		inline dense_vector operator + (const dense_vector& o) const {
			return dense_vector(head + o.head, tail + o.tail);
		}
		inline dense_vector operator - (const dense_vector& o) const {
			return dense_vector(head - o.head, tail - o.tail);
		}
		inline dense_vector operator * (T o) const {
			return dense_vector(head * o, tail * o);
		}
		inline dense_vector operator / (T o) const {
			return dense_vector(head / o, tail / o);
		}
		inline dense_vector& operator += (const dense_vector& o) {
			head += o.head;
			tail += o.tail;
			return *this;
		}
		inline dense_vector& operator -= (const dense_vector& o) {
			head -= o.head;
			tail -= o.tail;
			return *this;
		}
		inline dense_vector& operator *= (T o) {
			head *= o; tail *= o;
			return *this;
		}
		inline dense_vector& operator /= (T o) {
			head /= o; tail /= o;
			return *this;
		}

		inline T dot(const dense_vector& o) const {
			return head * o.head + tail.dot(o.tail);
		}
		
		friend inline T dot(const dense_vector& a,
							const dense_vector& b) {
			return a.dot(b);
		}
		
		friend inline T length(const dense_vector& a) {
			return std::sqrt(a.dot(a));
		}
		
		friend inline T normalize(const dense_vector& a) {
			return a / length(a);
		}
	};
	
	template <class T>
	class dense_vector<T, 0> {
	public:
		dense_vector() = default;
		dense_vector(const dense_vector&) = default;
		inline dense_vector operator + (const dense_vector&) const {
			return dense_vector();
		}
		inline dense_vector operator - (const dense_vector&) const {
			return dense_vector();
		}
		inline dense_vector operator * (T) const {
			return dense_vector();
		}
		inline dense_vector operator / (T) const {
			return dense_vector();
		}
		inline dense_vector& operator += (const dense_vector&) {
			return *this;
		}
		inline dense_vector& operator -= (const dense_vector&) {
			return *this;
		}
		inline dense_vector& operator *= (T) {
			return *this;
		}
		inline dense_vector& operator /= (T) {
			return *this;
		}
		inline T dot(const dense_vector&) const {
			return static_cast<T>(0);
		}
	};
	
	template <int I, class T, int N> struct dense_vector_getter {
		static T& get(dense_vector<T, N>& v) {
			return dense_vector_getter<I - 1, T, N - 1>::get(v.tail);
		}
		static const T& get(const dense_vector<T, N>& v) {
			return dense_vector_getter<I - 1, T, N - 1>::get(v.tail);
		}
	};
	template <class T, int N> struct dense_vector_getter<0, T, N> {
		static T& get(dense_vector<T, N>& v) {
			return v.head;
		}
		static const T& get(const dense_vector<T, N>& v) {
			return v.head;
		}
	};
	template <int I, class T, int N>
	T& get(dense_vector<T, N>& v) {
		return dense_vector_getter<I, T, N>::get(v);
	}
	template <int I, class T, int N>
	const T& get(const dense_vector<T, N>& v) {
		return dense_vector_getter<I, T, N>::get(v);
	}
	
	template <class T>
	dense_vector<T, 3> cross(const dense_vector<T, 3>& a,
			const dense_vector<T, 3>& b) {
		return dense_vector<T, 3>
		(get<1>(a) * get<2>(b) - get<2>(a) * get<1>(b),
		 get<2>(a) * get<0>(b) - get<0>(a) * get<2>(b),
		 get<0>(a) * get<1>(b) - get<1>(a) * get<0>(b));
	}
	
	template <class T>
	class quaternion {
		T _r, _i, _j, _k;
	public:
		quaternion() = default;
		inline quaternion(T r, T i1, T i2, T i3):
		_r(r), _i(i1), _j(i2), _k(i3) { }
		explicit inline quaternion(T r):
		quaternion(r,
				   static_cast<T>(0),
				   static_cast<T>(0),
				   static_cast<T>(0)) { }
		explicit inline quaternion(const std::complex<T>& c):
		quaternion(c.real(), c.imag(),
				   static_cast<T>(0),
				   static_cast<T>(0)) { }
		template <class X>
		explicit inline quaternion(const quaternion<X>& affecter):
		quaternion(static_cast<T>(affecter.R_component_1()),
				   static_cast<T>(affecter.R_component_2()),
				   static_cast<T>(affecter.R_component_3()),
				   static_cast<T>(affecter.R_component_4())) { }
		
		inline quaternion& operator = (T r) {
			_r = r; _i = _j = _k = static_cast<T>(0); return *this;
		}
		inline quaternion& operator = (const std::complex<T>& c) {
			_r = c.real(); _i = c.imag(); _j = _k = static_cast<T>(0);
			return *this;
		}
		inline quaternion& operator = (const quaternion&) = default;
		template <class X>
		inline quaternion& operator = (const quaternion<X>& affecter) {
			_r = static_cast<T>(affecter.R_component_1());
			_i = static_cast<T>(affecter.R_component_2());
			_j = static_cast<T>(affecter.R_component_3());
			_k = static_cast<T>(affecter.R_component_4());
			return *this;
		}
		
		inline const T& real() const { return _r; }
		inline T& real() { return _r; }
		
		inline const T& R_component_1() const { return _r; }
		inline T& R_component_1() { return _r; }
		inline const T& R_component_2() const { return _i; }
		inline T& R_component_2() { return _i; }
		inline const T& R_component_3() const { return _j; }
		inline T& R_component_3() { return _j; }
		inline const T& R_component_4() const { return _k; }
		inline T& R_component_4() { return _k; }
		
		inline std::complex<T> C_component_1() const {
			return std::complex<T>(_r, _i);
		}
		inline std::complex<T> C_component_2() const {
			return std::complex<T>(_j, _k);
		}
		
		inline quaternion unreal() const { return quaternion(T(0), _i, _j, _k); }
		
		inline quaternion operator + (const quaternion& o) const {
			return quaternion(_r + o._r, _i + o._i,
							  _j + o._j, _k + o._k);
		}
		inline quaternion operator - (const quaternion& o) const {
			return quaternion(_r - o._r, _i - o._i,
							  _j - o._j, _k - o._k);
		}
		inline quaternion operator * (const quaternion& o) const {
			return quaternion(_r * o._r - _i * o._i - _j * o._j - _k * o._k,
							  _r * o._i + _i * o._r + _j * o._k - _k * o._j,
							  _r * o._j - _i * o._k + _j * o._r + _k * o._i,
							  _r * o._k + _i * o._j - _j * o._i + _k * o._r);
		}
		inline quaternion& operator += (const quaternion& o) {
			return *this = *this + o;
		}
		inline quaternion& operator -= (const quaternion& o) {
			return *this = *this - o;
		}
		inline quaternion& operator *= (const quaternion& o) {
			return *this = *this * o;
		}
		
		/** Q^x (Q: quat, x: real) for spatial rotation quaternion */
		inline quaternion spatial_power(T v) const {
			auto theta = std::acosf(_r) * v;
			auto sq = std::sqrt(_i * _i + _j * _j + _k * _k);
			if (sq != static_cast<T>(0)) sq = static_cast<T>(1) / sq;
			auto newSin = std::sin(theta);
			sq *= newSin;
			return quaternion(std::cos(theta),
							  _i * sq, _j * sq, _k * sq);
		}
		
		friend inline quaternion real(const quaternion& o) {
			return o.R_component_1();
		}
		friend inline quaternion conj(const quaternion& o) {
			return quaternion(o._r, -o._i, -o._j, -o._k);
		}
		friend inline T abs(const quaternion& o) {
			return std::sqrt(o._r * o._r +
							 o._i * o._i +
							 o._j * o._j +
							 o._k * o._k);
		}
		friend inline T sup(const quaternion& o) {
			return std::max({
				std::abs(o._r), std::abs(o._i),
				std::abs(o._j), std::abs(o._k)
			});
		}
		friend inline quaternion slerp(const quaternion& a,
									   const quaternion& b,
									   T mix) {
			return (b * conj(a)).spatial_power(mix) * a;
		}
	};
	
	template <class TX, class T>
	class spline_curve {
		std::valarray<TX> X;
		std::valarray<T> Y;
		std::valarray<T> D;
		
	public:
		template <class It>
		spline_curve(It begin,
					 It end,
					 bool cyclic) {
			
			assert (!cyclic); // cyclic curve is not supported yet...
			
			X.resize(std::distance(begin, end));
			Y.resize(std::distance(begin, end));
			for (std::size_t i = 0; begin != end; ++begin, ++i) {
				X[i] = begin->first;
				Y[i] = begin->second;
			}
			
			auto N = Y.size();
			
			assert(N > 0);
			
			// linear interpolation
			if (N <= 2) {
				return;
			}
			
			D.resize(N);
			
			// tridiag matrix elements
			std::valarray<T> aa, bb, cc, E;
			aa.resize(N);
			bb.resize(N);
			cc.resize(N);
			E.resize(N);
			
			cc[0] = 0; aa[N - 1] = 0;
			for (std::size_t i = 0; i < N - 1; ++i) {
				auto d = TX(1) / (X[i + 1] - X[i]);
				cc[i + 1] = d;
				aa[i] = d;
			}
			
			for (std::size_t i = 0; i < N; ++i) {
				bb[i] = (aa[i] + cc[i]) * T(2);
			}
			
			E[0] = T(3) * (Y[1] - Y[0]);
			E[N - 1] = T(3) * (Y[N - 1] - Y[N - 2]);
			for (std::size_t i = 1; i < N - 1; ++i) {
				E[i] = T(3) * (Y[i + 1] - Y[i - 1]);
			}
			
			// solve
			detail::solve_diag(D, aa, bb, cc, E);
		}
		
		T operator () (TX x) const {
			if (x <= X[0] ||
				X.size() == 1) {
				return Y[0];
			} else if (x >= X[X.size() - 1]) {
				return Y[Y.size() - 1];
			} else if (Y.size() == 2) {
				return Y[0] + (Y[1] - Y[0]) *
				(x - X[0]) / (X[1] - X[0]);
			}
			
			auto it = std::upper_bound
			(std::begin(X), std::end(X), x) - 1;
			auto i = it - std::begin(X);
			
			auto a = Y[i];
			auto b = D[i];
			auto c = T(3) * (Y[i + 1] - Y[i]) -
			T(2) * D[i] - D[i + 1];
			auto d = T(2) * (Y[i] - Y[i + 1]) +
			D[i] + D[i + 1];
			auto f = (x - X[i]) / (X[i + 1] - X[i]);
			return ((d * f + c) * f + b) * f + a;
		}
	};
	
	
	
	template <class T>
	class quaternion_spline_curve {
	public:
		using quat_type = quaternion<T>;
		using vec_type = dense_vector<T, 3>;
		static inline T _0() { return static_cast<T>(0); }
		static inline T _1() { return static_cast<T>(1); }
		static inline T _2() { return static_cast<T>(2); }
	private:
		std::valarray<T> X;
		std::valarray<quat_type> Y;
		std::valarray<vec_type> W1;
		std::valarray<vec_type> W2;
		std::valarray<vec_type> W3;
		static vec_type quat_to_vec(const quat_type& q) {
			return vec_type(q.R_component_2(),
							q.R_component_3(),
							q.R_component_4());
		}
		
		static quat_type make_quat(const vec_type& v) {
			auto len = length(v);
			if (len == _0()) {
				return quat_type(_1(), _0(), _0(), _0());
			} else {
				auto vv = v / len;
				len /= _2();
				auto s = std::sin(len), c = std::cos(len);
				return quat_type(c,
								 get<0>(vv) * s,
								 get<1>(vv) * s,
								 get<2>(vv) * s);
			}
		}
		
		T compute_error(std::size_t begin,
						std::size_t end,
						const std::valarray<vec_type>& E,
						const std::valarray<vec_type>& nE) const {
			if (begin == end) return _0();
			else if (begin + 1 == end) {
				auto d = nE[begin] - E[begin];
				return dot(d, d);
			} else {
				auto mid = (begin + end) >> 1;
				return compute_error(begin, mid, E, nE) +
					   compute_error(mid, end, E, nE);
			}
		}
	public:
		template <class It>
		quaternion_spline_curve(It begin,
								It end,
								bool cyclic,
								T tolerance = static_cast<T>(1.e-6)) {
			
			assert (!cyclic); // cyclic curve is not supported yet...
			
			X.resize(std::distance(begin, end));
			Y.resize(std::distance(begin, end));
			for (std::size_t i = 0; begin != end; ++begin, ++i) {
				X[i] = begin->first;
				Y[i] = begin->second;
			}
			
			auto N = Y.size();
			
			assert(N > 0);
			
			// linear interpolation
			if (N <= 2) {
				return;
			}
			
			// differences
			std::valarray<T> thetas;
			std::valarray<vec_type> axises;
			thetas.resize(N - 1);
			axises.resize(N - 1);
			for (std::size_t i = 0; i < N - 1; ++i) {
				auto diff = conj(Y[i]) * Y[i + 1];
				auto vec = quat_to_vec(diff);
				auto len = length(vec);
				if (len != _0()) {
					vec /= len;
				}
				axises[i] = vec;
				
				auto cosVal = diff.real();
				cosVal = std::max(std::min(cosVal, _1()), -_1());
				thetas[i] = std::acos(cosVal) * _2();
			}
			
			// B matrix (column-major)
			std::valarray<vec_type> BF;
			std::valarray<vec_type> BI;
			BF.resize(N * 3);
			BI.resize(N * 3);
			for (std::size_t i = 0; i < N - 1; ++i) {
				if (thetas[i] == _0() ||
					(get<0>(axises[i]) == _0() &&
					 get<1>(axises[i]) == _0() &&
					 get<2>(axises[i]) == _0())) {
					BF[i * 3 + 0] = BI[i * 3 + 0] = vec_type(_1(), _0(), _0());
					BF[i * 3 + 1] = BI[i * 3 + 1] = vec_type(_0(), _1(), _0());
					BF[i * 3 + 2] = BI[i * 3 + 1] = vec_type(_0(), _0(), _1());
				} else {
					auto s = std::sin(thetas[i]);
					auto c = std::cos(thetas[i]);
					auto e = axises[i];
					auto e1 = get<0>(e);
					auto e2 = get<1>(e);
					auto e3 = get<2>(e);
					auto a = s / thetas[i];
					auto b = (_1() - c) / thetas[i];
					BF[i * 3 + 0] = vec_type
					(e1 * e1 + a * (e2 * e2 + e3 * e3),
					 e1 * e2 - a * e1 * e2 - b * e3,
					 e1 * e3 - a * e1 * e3 - b * e2);
					BF[i * 3 + 1] = vec_type
					(e1 * e2 - a * e1 * e2 + b * e3,
					 e2 * e2 + a * (e1 * e1 + e3 * e3),
					 e2 * e3 - a * e2 * e3 - b * e1);
					BF[i * 3 + 2] = vec_type
					(e1 * e3 - a * e1 * e3 - b * e2,
					 e2 * e3 - a * e2 * e3 + b * e1,
					 e3 * e3 + a * (e1 * e1 + e2 * e2));
					a = s * thetas[i] / (_2() * (_1() - c));
					b = thetas[i] / _2();
					BI[i * 3 + 0] = vec_type
					(e1 * e1 + a * (e2 * e2 + e3 * e3),
					 e1 * e2 - a * e1 * e2 + b * e3,
					 e1 * e3 - a * e1 * e3 + b * e2);
					BI[i * 3 + 1] = vec_type
					(e1 * e2 - a * e1 * e2 - b * e3,
					 e2 * e2 + a * (e1 * e1 + e3 * e3),
					 e2 * e3 - a * e2 * e3 + b * e1);
					BI[i * 3 + 2] = vec_type
					(e1 * e3 - a * e1 * e3 + b * e2,
					 e2 * e3 - a * e2 * e3 - b * e1,
					 e3 * e3 + a * (e1 * e1 + e2 * e2));
				}
			}
			
			// tridiag matrix scaler
			// aa: [1 ... N - 3]
			// cc: [2 ... N - 2]
			std::valarray<T> aa, bb, cc;
			aa.resize(N);
			bb.resize(N);
			cc.resize(N);
			
			cc[1] = 0; aa[N - 2] = 0;
			for (std::size_t i = 0; i < N - 1; ++i) {
				auto d = _2() / (X[i + 1] - X[i]);
				cc[i + 1] = d;
				aa[i] = d;
			}
			
			// initial/final velocity
			std::valarray<vec_type> E;
			E.resize(N);
			E[0]     = vec_type(_0(), _0(), _0());
			E[N - 1] = vec_type(_0(), _0(), _0());
			
			// base D value (without non-linear term)
			// DB: [1 ... N - 2]
			std::valarray<vec_type> DB;
			DB.resize(N);
			for (std::size_t i = 0; i < N - 1; ++i) {
				auto scale = aa[i] * static_cast<T>(3); // reuse
				auto v = axises[i] * (thetas[i] * scale);
				DB[i] = v;
			}
			for (std::size_t i = N - 2; i > 0; --i) {
				DB[i] += DB[i - 1];
			}
			
			// apply initial/final velocities
			{
				const auto& w = E[0];
				DB[1] -= BF[0] * (aa[1] * get<0>(w));
				DB[1] -= BF[1] * (aa[1] * get<1>(w));
				DB[1] -= BF[2] * (aa[1] * get<2>(w));
			}
			{
				const auto& w = E[N - 1];
				DB[N - 2] -= BI[N * 3 - 6] * (aa[N - 2] * get<0>(w));
				DB[N - 2] -= BI[N * 3 - 5] * (aa[N - 2] * get<1>(w));
				DB[N - 2] -= BI[N * 3 - 4] * (aa[N - 2] * get<2>(w));
			}
			
			// precomputed values for non-linear term
			std::valarray<std::pair<T, T>> Rparam;
			Rparam.resize(N - 1);
			for (std::size_t i = 1; i < N - 1; ++i) {
				auto t = thetas[i - 1];
				auto s = std::sin(t), c = std::cos(t);
				auto _a = _1() - c;
				auto _b = _2() * _a;
				Rparam[i] = std::make_pair
				((t - s) / _b,
				 (t * s - _b) / (t * _a));
			}
			
			const auto& A = aa;
			const auto& C = cc;
			std::valarray<T> B;
			B.resize(N);
			
			std::valarray<vec_type> D;
			D.resize(N);
			
			std::valarray<vec_type> nE;
			nE.resize(N);
			nE[0] = E[0];
			nE[N - 1] = E[N - 1];
			
			// first iteration: D includes no non-linear terms
			for (std::size_t i = 0; i < N; ++i) {
				D[i] = DB[i];
			}
			
			// iterations
			for (std::size_t j = 0; j < 65536; ++j) {
				for (std::size_t i = 1; i < N - 1; ++i) {
					B[i] = (A[i] + C[i]) * T(2);
				}
				
				// solve
				for (std::size_t i = 2; i < N - 1; ++i) {
					auto m = A[i] / B[i - 1];
					B[i] -= m * C[i - 1];
					
					const auto& pd = D[i - 1];
					D[i] -= (BF[i * 3 - 3] * get<0>(pd) +
							 BF[i * 3 - 2] * get<1>(pd) +
							 BF[i * 3 - 1] * get<2>(pd)) * m;
				}
				
				nE[N - 2] = D[N - 2] / B[N - 2];
				for (std::size_t i = N - 2; i > 1; --i) {
					const auto& pe = nE[i];
					nE[i - 1] = (D[i - 1] - 
								 (BI[i * 3 - 3] * get<0>(pe) +
								  BI[i * 3 - 2] * get<1>(pe) +
								  BI[i * 3 - 1] * get<2>(pe)) * C[i - 1])
								/ B[i - 1];
				}
				
				// evaluate error
				if (j > 0) {
					auto error = compute_error(1, N - 1, E, nE);
					std::copy(std::begin(nE), std::end(nE),
							  std::begin(E));
					if (error < tolerance) {
						// convergence
						break;
					} else {
						// too much error...
					}
				} else {
					std::copy(std::begin(nE), std::end(nE),
							  std::begin(E));
				}
				for (std::size_t i = 1; i < N - 1; ++i) {
					const auto& param = Rparam[i];
					const auto& axis = axises[i - 1];
					const auto& pe = nE[i];
					
					auto _a = dot(pe, pe);
					auto _b = dot(pe, axis);
					auto R = axis * (std::get<0>(param) * (_a - _b * _b));
					R += cross(cross(axis, pe), axis) * (std::get<1>(param) * _b);
					
					D[i] = DB[i] - R;
				}
			}
			
			// precompute coefs
			W1.resize(N - 1);
			W2.resize(N - 1);
			W3.resize(N - 1);
			for (std::size_t i = 0; i < N - 1; ++i) {
				const auto& e1 = E[i], e2 = E[i + 1];
				W1[i] = e1;
				W3[i] = axises[i] * thetas[i];
				W2[i] = BI[i * 3 + 0] * get<0>(e2) +
						BI[i * 3 + 1] * get<1>(e2) +
						BI[i * 3 + 2] * get<2>(e2) -
						W3[i] * static_cast<T>(3);
			}
			
		}
		
		quat_type operator () (T x) const {
			if (x <= X[0] ||
				X.size() == 1) {
				return Y[0];
			} else if (x >= X[X.size() - 1]) {
				return Y[Y.size() - 1];
			} else if (Y.size() == 2) {
				return slerp(Y[0], Y[1], (x - X[0]) / (X[1] - X[0]));
			}
			
			auto it = std::upper_bound
			(std::begin(X), std::end(X), x) - 1;
			auto i = it - std::begin(X);
			assert(i < X.size() - 1);
			auto f = (x - X[i]) / (X[i + 1] - X[i]);
			auto ff = f - _1();
			auto vec = W1[i] * (ff*ff*f) + W2[i] * (ff*f*f) + W3[i] * (f*f*f);
			
			auto quat = make_quat(vec);
			return Y[i] * quat;
		}
	};
}
