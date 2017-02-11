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

#pragma once

#include <array>
#include <cstdlib>
#include <cstring>
#include <string>

#include "Exception.h"
#include "Math.h"

namespace spades {
	std::string Intern(const std::string &);

	template <typename T> std::string ToString(const T &v) { return std::to_string(v); }

	template <typename F> std::size_t StringSpan(const char *str, F predicate) {
		auto *ptr = str;
		while (*ptr != 0 && predicate(*ptr))
			ptr++;
		return ptr - str;
	}

	class StandardTokenizer {
		const char *ptr;

	public:
		StandardTokenizer(const char *ptr);
		class Iterator {
			const char *ptr;
			StandardTokenizer *tokenizer;
			std::string token;
			std::string prevToken;
			bool nowPrev;
			void SkipWhitespace();
			void AnalyzeToken();

		public:
			Iterator(const char *ptr, StandardTokenizer *tokenizer);
			std::string operator*();
			void operator++();
			void operator--();
			bool operator==(const Iterator &it) const {
				return ptr == it.ptr && nowPrev == it.nowPrev;
			}
			bool operator!=(const Iterator &it) const {
				return ptr != it.ptr || nowPrev != it.nowPrev;
			}
			const char *GetPointer() const { return ptr - token.size(); }
			StandardTokenizer &GetTokenizer() const { return *tokenizer; }
		};
		Iterator begin();
		Iterator end();
		const char *GetString() { return ptr; }
	};

	template <typename T, int N = 32> class StaticArray {
		std::array<T, N> staticArray;
		std::vector<T> dynamicArray;
		std::size_t count;

	public:
		StaticArray() : count(0) {}
		std::size_t size() const { return count; }
		void push_back(const T &e) {
			if (count == N) {
				dynamicArray.push_back(e);
				count++;
			} else {
				staticArray[count++] = e;
			}
		}
		void push_back(T &&e) {
			if (count == N) {
				dynamicArray.push_back(e);
				count++;
			} else {
				staticArray[count++] = e;
			}
		}
		T &operator[](std::size_t i) {
			if (i < N)
				return staticArray[i];
			else
				return dynamicArray[i - N];
		}
		const T &operator[](std::size_t i) const {
			if (i < N)
				return staticArray[i];
			else
				return dynamicArray[i - N];
		}
		class iterator {
			StaticArray<T, N> &arr;
			mutable std::size_t index;
			iterator(StaticArray<T, N> &arr, std::size_t index) : arr(arr), index(index) {}
			T &operator*() { return arr[index]; }
			const T &operator*() const { return arr[index]; }
			iterator operator+(std::size_t i) { return iterator(arr, index + i); }
			iterator operator-(std::size_t i) { return iterator(arr, index - i); }
			const iterator operator+(std::size_t i) const { return iterator(arr, index + i); }
			const iterator operator-(std::size_t i) const { return iterator(arr, index - i); }
			void operator++() const { ++index; }
			void operator--() const { --index; }
			T &operator[](std::size_t i) { return arr[index + i]; }
			const T &operator[](std::size_t i) const { return arr[index + i]; }
			bool operator==(const iterator &it) const {
				return &arr == &it.arr && index == it.index;
			}
			bool operator!=(const iterator &it) const {
				return &arr != &it.arr || index != it.index;
			}
		};
		typedef const iterator const_iterator;
		iterator begin() { return iterator(*this, 0); }
		iterator end() { return iterator(*this, size()); }
		const_iterator begin() const { return iterator(*this, 0); }
		const_iterator end() const { return iterator(*this, size()); }
	};

	template <std::size_t numArgs> class Formatter {
		struct Segment {
			std::size_t start;
			std::size_t length;
		};
		struct Placeholder {
			std::size_t parameterIndex;
			std::string format;
		};

		const char *str;
		StaticArray<Segment> segments;
		StaticArray<Placeholder> placeholders;
		std::array<std::string, numArgs> parameters;

	public:
		Formatter(const char *str) : str(str) {
			auto *firstPlaceholder = std::strchr(str, '{');
			if (firstPlaceholder) {
				std::size_t index = firstPlaceholder - str;
				segments.push_back({0, index});
				while (str[index] != '\0') {
					// parse placeholder.
					index++;
					auto endIndex = index + std::strcspn(str + index, ":}");
					if (str[endIndex] == '\0')
						SPRaise("Malformed format string (placeholder is not closed): %s", str);
					char *endptr;
					auto idx = static_cast<std::size_t>(strtol(str + index, &endptr, 10));
					if (endptr != str + endIndex) {
						SPRaise("Malformed format string (failed to parse parameter index): %s",
						        str);
					}
					Placeholder p;
					p.parameterIndex = idx;
					if (idx >= numArgs) {
						SPRaise(
						  "Malformed format string (using non-existent parameter index %d): %s",
						  static_cast<int>(idx), str);
					}
					if (str[endIndex] == ':') {
						index = endIndex + 1;
						endptr = const_cast<char *>(std::strchr(str + index, '}'));
						if (!endptr)
							SPRaise("Malformed format string (placeholder is not closed): %s", str);
						endIndex = endptr - str;
						p.format = std::string(str + index, endIndex - index);
						index = endIndex + 1;
					} else {
						index = endIndex + 1;
					}
					placeholders.push_back(p);

					// find next placeholder
					auto *nextPlaceholder = std::strchr(str + index, '{');
					if (nextPlaceholder) {
						endIndex = nextPlaceholder - str;
					} else {
						endIndex = std::strlen(str);
					}
					segments.push_back({index, endIndex - index});
					index = endIndex;
				}
			} else {
				segments.push_back({0, std::strlen(str)});
			}
		}

	private:
		template <std::size_t index = 0> void SetParameterValue() {
			static_assert(index <= numArgs, "index <= numArgs");
		}
		template <std::size_t index = 0, class Head, class... T>
		void SetParameterValue(Head head, T... args) {
			static_assert(index < numArgs, "index < numArgs");
			parameters[index] = ToString(head);
			SetParameterValue<index + 1, T...>(args...);
		}

	public:
		template <class... T> std::string Format(T... args) {
			SetParameterValue<>(args...);
			std::string ret;
			ret.append(str + segments[0].start, segments[0].length);
			for (std::size_t i = 0; i < placeholders.size(); i++) {
				ret += parameters[placeholders[i].parameterIndex];
				ret.append(str + segments[i + 1].start, segments[i + 1].length);
			}
			return ret;
		}
	};

	template <> class Formatter<0> {
		const char *fmt;

	public:
		Formatter(const char *fmt) : fmt(fmt) {
			if (strchr(fmt, '{')) {
				SPRaise("Malformed format string (using non-existent parameter. no parameter "
				        "provided): %s",
				        fmt);
			}
		}
		std::string Format() { return fmt; }
	};

	template <class... T> std::string Format(const char *str, T... args) {
		return Formatter<sizeof...(args)>(str).Format(args...);
	}

	template <class... T> std::string Format(const std::string &str, T... args) {
		return Format(str.c_str(), args...);
	}

	template <> std::string ToString<std::string>(const std::string &s);
	template <> std::string ToString<const char *>(const char *const &s);
	template <> std::string ToString<Vector2>(const Vector2 &v);
	template <> std::string ToString<Vector3>(const Vector3 &v);
	template <> std::string ToString<Vector4>(const Vector4 &v);
	template <> std::string ToString<IntVector3>(const IntVector3 &v);

	// `CheckPlural` converts the given value to an integer for plural form identification.
	// Let's ignore huge numbers for now...
	template <class... T> int CheckPlural(T... args) { return 1; }
	template <class... T> int CheckPlural(int v, T... args) { return v; }
	template <class... T> int CheckPlural(long v, T... args) { return static_cast<int>(v); }
	template <class... T> int CheckPlural(unsigned int v, T... args) { return v; }
	template <class... T> int CheckPlural(unsigned long v, T... args) {
		return static_cast<int>(v);
	}
	template <class... T> int CheckPlural(short v, T... args) { return v; }
	template <class... T> int CheckPlural(unsigned short v, T... args) { return v; }
	template <class... T> int CheckPlural(char v, T... args) { return v; }
	template <class... T> int CheckPlural(unsigned char v, T... args) { return v; }

	std::string GetTextRaw(const std::string &domain, const std::string &ctx,
	                       const std::string &text, int plural);
	std::string GetTextRawPlural(const std::string &domain, const std::string &ctx,
	                             const std::string &text, const std::string &textPlural,
	                             int plural);

	template <class... T>
	std::string GetText(const std::string &domain, const std::string &context,
	                    const std::string &text, T... args) {
		int plural = CheckPlural(args...);
		auto s = GetTextRaw(domain, context, text, plural);
		return Format(s, args...);
	}

	template <class... T>
	std::string GetTextPlural(const std::string &domain, const std::string &context,
	                          const std::string &text, const std::string &textPl, T... args) {
		int plural = CheckPlural(args...);
		auto s = GetTextRawPlural(domain, context, text, textPl, plural);
		return Format(s, args...);
	}

	class CatalogDomainHandle {
		std::string domain;

	public:
		CatalogDomainHandle(const std::string &domainName);
		template <class... T>
		std::string Get(const std::string &context, const std::string &text, T... args) {
			return GetText(domain, context, text, args...);
		}
		template <class... T>
		std::string GetPlural(const std::string &context, const std::string &text,
		                      const std::string &textPl, T... args) {
			return GetTextPlural(domain, context, text, textPl, args...);
		}
	};

	extern CatalogDomainHandle defaultDomain;

	void LoadCurrentLocale();

	/**
	 * Returns a current local identifier in this format: `[language[_territory]]`.
	 */
	std::string GetCurrentLocaleAndRegion();
}

#define _Tr(...) ::spades::defaultDomain.Get(__VA_ARGS__)
#define _TrN(...) ::spades::defaultDomain.GetPlural(__VA_ARGS__)
#define SPADES_DEFINE_GETTEXT_DOMAIN(name) static CatalogDomainHandle _##name(#name)
