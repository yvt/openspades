/*
 Happy Template Metaprogramming Library.
 
 Copyright (c) 2014 yvt
 
 Licensed with MIT license.
 */
#include <iostream>
#include <iterator>

namespace stmp {
	
	
	class type_list_null {
	public:
		static const bool is_type_list = true;
		static const bool is_type_list_empty = true;
	};
	
	template<class T, class Next>
	class type_list {
	public:
		static const bool is_type_list = true;
		static const bool is_type_list_empty = false;
		static_assert(Next::is_type_list, "Next is not type_list");
		typedef T hd;
		typedef Next tl;
	};
	
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
		virtual void visit(typename L::hd&) = 0;
	};
	template<> class visitor_generator<type_list_null> {};
	
	
	template<class L>
	class const_visitor_generator : public const_visitor_generator<typename L::tl> {
	public:
		virtual void visit(const typename L::hd&) = 0;
	};
	template<> class const_visitor_generator<type_list_null> {};
	
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
		constexpr find_type_list(const Predicate& pred, const ResultMap& func):
		pred(pred), func(func) {}
		constexpr auto evaluate() const -> decltype(func.template evaluate<typename List::hd>()) {
			return pred.template evaluate<typename List::hd>() ?
			func.template evaluate<typename List::hd>() :
			find_type_list<Predicate, ResultMap, typename List::tl>(pred, func).evaluate();
		}
	};
	
	template<class Predicate, class ResultMap>
	class find_type_list<Predicate, ResultMap, type_list_null> {
		ResultMap func;
	public:
		constexpr find_type_list(const Predicate&, const ResultMap& func):
		func(func) {}
		constexpr auto evaluate() const -> decltype(func.not_found()) {
			return func.not_found();
		}
	};
	
	
	
	// constant look-up table generated in compile-time.
	// see also: http://e.yvt.jp/#!20c6ce2d9c4b9f0f4937c8cfdad02ea4390f745c
	template <class TGen, std::size_t tableLen>
	class static_table {
		using T = decltype((*(TGen*)nullptr)[0]);
		
		template <std::size_t begin, std::size_t len>
		struct part {
			static const int half = len >> 1;
			part<begin, half> first;
			part<begin + half, len - half> second;
			constexpr part(const TGen& gen):
			first(gen), second(gen) { }
			
			constexpr const T& get(std::size_t index) const {
				return index < begin + len ? first.get(index) : second.get(index);
			}
		};
		
		template<std::size_t index>
		struct part<index, 1>{
			T e0;
			constexpr part(const TGen& gen):
			e0(gen[index])
			{ }
			constexpr const T& get(std::size_t) const { return e0; }
		};
		
		template<std::size_t index>
		struct part<index, 2>{
			T e0, e1;
			constexpr part(const TGen& gen):
			e0(gen[index]), e1(gen[index + 1])
			{ }
			
			constexpr const T& get(std::size_t i) const {
				return i <= index ? e0 : e1 ;
			}
		};
		
		template<std::size_t index>
		struct part<index, 0>{
			static_assert(tableLen == 0, "len of a part became zero");
			constexpr part(const TGen&) { }
		};
		
		
		part<0, tableLen> parts;
		
	public:
		constexpr static_table(const TGen& gen):
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
		constexpr float smoothstep(float v) const {
			return v * v * (3.f - 2.f * v);
		}
	public:
		constexpr float operator [](std::size_t index) const {
			return smoothstep( static_cast<float>(index) / 15.f );
		}
	};

	
}
