/*
 Happy Template Metaprogramming Library.
 
 Copyright (c) 2014 yvt
 
 Licensed with MIT license.
 */
#include <iostream>
#include <iterator>
#include <type_traits>
#include <memory>
#include <cassert>

#define HAS_CONSTEXPR 1

#ifdef _MSC_VER
#undef HAS_CONSTEXPR
#define HAS_CONSTEXPR 0
#endif

#if !HAS_CONSTEXPR
#define constexpr
#endif


namespace stmp {
	
	// creating our own version because boost is overweighted
	// (preproecssing optional.hpp emits 50000 lines of C++ code!)
	// `optional` can be defined in SML as following:
	// datatype optional 'T = None | Some of 'T
	// and corresponding type in .Net Framework is System.Nullable<T>.
	template<class T>
	class optional {
		std::aligned_storage<sizeof(T), alignof(T)> storage;
		bool has_some;
		using Allocator = std::allocator<T>;
	public:
		optional():has_some(false){}
		optional(optional& o):has_some(o.has_some) {
			if(has_some) { Allocator().construct(get_pointer(), o.get()); }
		}
		optional(const optional& o):has_some(o.has_some) {
			if(has_some) { Allocator().construct(get_pointer(), o.get()); }
		}
		optional(optional&& o):has_some(o.has_some) {
			if(has_some) {
				Allocator().construct(get_pointer(), std::move(o.get()));
				o.has_some = false;
			}
		}
		~optional() {reset();}
		void reset() {
			if(has_some) {
				Allocator().destroy(get_pointer());
				has_some = false;
			}
		}
		template<class... Args>
		void reset(Args&&... args) {
			reset();
			Allocator().construct(get_pointer(), std::forward<Args>(args)...);
			has_some = true;
		}
		template<class U>
		void operator =(U&& o) {
			reset(std::forward<U>(o));
		}
		
		T *get_pointer() { return has_some ? reinterpret_cast<T *>(&storage) : nullptr; }
		const T *get_pointer() const { return has_some ? reinterpret_cast<const T *>(&storage) : nullptr; }
		T& get() { assert(has_some); return *get_pointer(); }
		const T& get() const { assert(has_some); return *get_pointer(); }
		T& operator ->() { assert(has_some); return get(); }
		const T& operator ->() const { assert(has_some); return get(); }
		
		T& operator *() { assert(has_some); return get(); }
		const T& operator *() const { assert(has_some); return get(); }
		
		explicit operator bool() const { return has_some; }
	};
	
	/** Empty singly-linked list of types. */
	class type_list_null {
	public:
		static const bool is_type_list = true;
		static const bool is_type_list_empty = true;
	};
	
	/** Singly-linked list of types, having at least one type. */
	template<class T, class Next>
	class type_list {
	public:
		static const bool is_type_list = true;
		static const bool is_type_list_empty = false;
		static_assert(Next::is_type_list, "Next is not type_list");
		typedef T hd;
		typedef Next tl;
	};
	
	/** creates type_list using variadic template arguments. */
	template<class Head, class ...Tail>
	class make_type_list {
	public:
		typedef type_list<Head, typename make_type_list<Tail...>::list> list;
	};
	
	template<class Head>
	class make_type_list<Head> {
	public:
		typedef type_list<Head, type_list_null> list;
	};
	
	template<class L>
	class visitor_generator : public visitor_generator<typename L::tl> {
	public:
		using visitor_generator<typename L::tl>::visit;
		virtual void visit(typename L::hd&) = 0;
	};
	template<class T>
	class visitor_generator<type_list<T, type_list_null>> {
	public:
		virtual void visit(T&) = 0;
	};
	template<> class visitor_generator<type_list_null> {};
	
	
	template<class L>
	class const_visitor_generator : public const_visitor_generator<typename L::tl> {
	public:
		using const_visitor_generator<typename L::tl>::visit;
		virtual void visit(const typename L::hd&) = 0;
	};
	template<class T>
	class const_visitor_generator<type_list<T, type_list_null>> {
	public:
		virtual void visit(const T&) = 0;
	};
	template<> class const_visitor_generator<type_list_null> {};
	
	/** static_find_type_list returns not_found_type when no matching type was found. */
	class not_found_type { };
	
	template<template<class> class Predicate, class List>
	class static_find_type_list {
	public:
		typedef typename static_find_type_list<Predicate, typename List::tl>::result result;
	};
	template<template<class> class Predicate>
	class static_find_type_list<Predicate, type_list_null> {
	public:
		typedef not_found_type result;
	};
	template<template<class> class Predicate, class Head, class Tail, bool predicateResult>
	struct static_find_type_list_internal { typedef Head result; };
	template<template<class> class Predicate, class Head, class Tail>
	struct static_find_type_list_internal<Predicate, Head, Tail, false> { typedef typename static_find_type_list<Predicate, Tail>::result result; };
	template<template<class> class Predicate, class Head, class Tail>
	class static_find_type_list<Predicate, type_list<Head, Tail>> {
	public:
		typedef typename static_find_type_list_internal<Predicate, Head, Tail, Predicate<Head>::result>::result result;
	};
	
	template<class Predicate, class ResultMap, class List>
	class find_type_list {
		Predicate pred;
		ResultMap func;
	public:
		inline constexpr find_type_list(const Predicate& pred, const ResultMap& func):
		pred(pred), func(func) {}
		inline constexpr auto evaluate() const -> decltype(func.template evaluate<typename List::hd>()) {
			return pred.template evaluate<typename List::hd>() ?
			func.template evaluate<typename List::hd>() :
			find_type_list<Predicate, ResultMap, typename List::tl>(pred, func).evaluate();
		}
	};
	
	template<class Predicate, class ResultMap>
	class find_type_list<Predicate, ResultMap, type_list_null> {
		ResultMap func;
	public:
		inline constexpr find_type_list(const Predicate&, const ResultMap& func):
		func(func) {}
		inline constexpr auto evaluate() const -> decltype(func.not_found()) {
			return func.not_found();
		}
	};
	
	
	
	/** constant look-up table generated in compile-time.
	 * generated table might be stored in the program image when assigned to constexpr variable.
	 * see also: http://e.yvt.jp/#!20c6ce2d9c4b9f0f4937c8cfdad02ea4390f745c */
	template <class TGen, std::size_t tableLen>
	class static_table {
		using T = decltype((*(TGen*)nullptr)[0]);
		
		template <std::size_t begin, std::size_t len>
		struct part {
			static const int half = len >> 1;
			part<begin, half> first;
			part<begin + half, len - half> second;
			inline constexpr part(const TGen& gen):
			first(gen), second(gen) { }
			
			inline constexpr const T& get(std::size_t index) const {
				return index < begin + len ? first.get(index) : second.get(index);
			}
		};
		
		template<std::size_t index>
		struct part<index, 1>{
			T e0;
			inline constexpr part(const TGen& gen):
			e0(gen[index])
			{ }
			inline constexpr const T& get(std::size_t) const { return e0; }
		};
		
		template<std::size_t index>
		struct part<index, 2>{
			T e0, e1;
			inline constexpr part(const TGen& gen):
			e0(gen[index]), e1(gen[index + 1])
			{ }
			
			inline constexpr const T& get(std::size_t i) const {
				return i <= index ? e0 : e1 ;
			}
		};
		
		template<std::size_t index>
		struct part<index, 0>{
			static_assert(tableLen == 0, "len of a part became zero");
			inline constexpr part(const TGen&) { }
		};
		
		
		part<0, tableLen> parts;
		
	public:
		inline constexpr static_table(const TGen& gen):
		parts(gen)
		{ }
		
		using const_reverse_iterator = std::reverse_iterator<const T *>;
		
		inline constexpr std::size_t size() const { return tableLen; }
		inline const T *data() const { return reinterpret_cast<const T *>(&parts); }
		inline const T *begin() const { return data(); }
		inline const T *end() const { return data() + size(); }
		inline const T *rbegin() const { return const_reverse_iterator(data() + size() - 1); }
		inline const T *rend() const { return const_reverse_iterator(data() - 1); }
		
		inline const T& operator [] (std::size_t index) const { return data()[index]; }
		
		template<std::size_t index> constexpr inline const T& get() const {
			static_assert(index < tableLen, "out of bounds");
			return parts.get(index);
		}
		
		
	};
	
	template<std::size_t len, class T>
	static inline constexpr auto make_static_table(const T& generator)
	-> static_table<T, len> {
		return static_table<T, len>(generator);
	}
	
	class example_static_table_generator {
		inline constexpr float smoothstep(float v) const {
			return v * v * (3.f - 2.f * v);
		}
	public:
		inline constexpr float operator [](std::size_t index) const {
			return smoothstep( static_cast<float>(index) / 15.f );
		}
	};

	
}
