/*
 * Copyright (c) 2013 yvt
 * WTFPL
 */

#include <cstdint>
#include <string>

#include <Core/Math.h>

namespace spades {
	class CP437 {
		CP437() = delete;
		~CP437() = delete;

	public:
		static char EncodeChar(std::uint32_t unicode, char fallback = 0xff);
		static std::uint32_t DecodeChar(char c);
		static std::string Encode(const std::string &, char fallback = 0xff);
		static std::string Decode(const std::string &);
	};
} // namespace spades
