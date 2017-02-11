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

// lm: both unordered_set and unordered_map have a _Tr typedef that conflicts with the define from
// String.h
//	so on msvc they need to be included before Strings.h
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "DynamicLibrary.h"
#include "FileManager.h"
#include "IStream.h"
#include "Settings.h"
#include "Strings.h"
#include <Core/Debug.h>

DEFINE_SPADES_SETTING(core_locale, "");

#ifdef WIN32
// FIMXE: not tested
static std::string GetUserLocale() {
	SPADES_MARK_FUNCTION();
	spades::DynamicLibrary kernel32("kernel32");
	auto *GetUserDefaultLocaleName = reinterpret_cast<int(__stdcall *)(wchar_t *, int)>(
	  kernel32.GetSymbolOrNull("GetUserDefaultLocaleName"));
	if (GetUserDefaultLocaleName) {
		char buf[256];
		wchar_t wbuf[256];
		if (!GetUserDefaultLocaleName(wbuf, 256)) {
			SPLog("Failed to get the user locale using GetUserDefaultLocaleName.");
		}
		std::wcstombs(buf, wbuf, 256);
		return buf;
	} else {
		SPLog("GetUserDefaultLocaleName not available. Defaults to C locale.");
		return "C";
	}
}
#else
#include <clocale>
static std::string GetUserLocale() {
	SPADES_MARK_FUNCTION();
	setlocale(LC_ALL, "");
	return setlocale(LC_MESSAGES, nullptr);
}
#endif

namespace spades {

	static std::unordered_set<std::string> internedStrings;

	std::string Intern(const std::string &s) { return *internedStrings.insert(s).first; }

	template <> std::string ToString<std::string>(const std::string &s) { return s; }
	template <> std::string ToString<const char *>(const char *const &s) { return s; }
	template <> std::string ToString<Vector2>(const Vector2 &v) {
		return Format("[{0}, {1}]", v.x, v.y);
	}
	template <> std::string ToString<Vector3>(const Vector3 &v) {
		return Format("[{0}, {1}, {2}]", v.x, v.y, v.z);
	}
	template <> std::string ToString<Vector4>(const Vector4 &v) {
		return Format("[{0}, {1}, {2}, {3}]", v.x, v.y, v.z, v.w);
	}
	template <> std::string ToString<IntVector3>(const IntVector3 &v);

	StandardTokenizer::StandardTokenizer(const char *ptr) : ptr(ptr) {}

	StandardTokenizer::Iterator StandardTokenizer::begin() { return Iterator(ptr, this); }
	StandardTokenizer::Iterator StandardTokenizer::end() {
		return Iterator(ptr + std::strlen(ptr), this);
	}

	StandardTokenizer::Iterator::Iterator(const char *ptr, StandardTokenizer *tokenizer)
	    : ptr(ptr), tokenizer(tokenizer), nowPrev(false) {
		if (*ptr) {
			SkipWhitespace();
			AnalyzeToken();
		}
	}

	std::string StandardTokenizer::Iterator::operator*() {
		if (nowPrev)
			return prevToken;
		return token;
	}
	void StandardTokenizer::Iterator::operator++() {
		if (nowPrev) {
			nowPrev = false;
			return;
		}
		prevToken = token;
		SkipWhitespace();
		AnalyzeToken();
	}
	void StandardTokenizer::Iterator::operator--() {
		if (nowPrev || prevToken.size() == 0) {
			SPRaise("Cannot rewind iterator further");
		}
		nowPrev = true;
	}

	void StandardTokenizer::Iterator::SkipWhitespace() {
		while (*ptr == ' ' || *ptr == '\n' || *ptr == '\r' || *ptr == '\t')
			ptr++;
	}

	void StandardTokenizer::Iterator::AnalyzeToken() {
		const char *endptr;
		if (*ptr == 0) {
			token.clear();
			return;
		} else if (*ptr >= '0' && *ptr <= '9') {
			endptr =
			  ptr + StringSpan(ptr, [](char c) { return c == '.' || (c >= '0' && c <= '9'); });
		} else if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || (*ptr == '_')) {
			endptr = ptr + StringSpan(ptr, [](char c) {
				         return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') ||
				                (c >= '0' && c <= '9');
				     });
		} else {
			endptr = ptr + 1;
		}
		token = std::string(ptr, endptr - ptr);
		ptr = endptr;
	}

	class PluralSelector {
		enum class OpCode : uint16_t {
			Jump,
			JumpIfZero,
			Add,
			Subtract,
			Muliply,
			Divide,
			Modulus,
			Negate,
			ShiftLeft,
			ShiftRight,
			BitwiseAnd,
			BitwiseOr,
			BitwiseXor,
			BitwiseNot,
			And,
			Or,
			Not,
			Less,
			Greater,
			LessOrEqual,
			GreaterOrEqual,
			Equality,
			Inequality,
			LoadConstant,
			LoadParam
		};
		struct Instruction {
			OpCode op;
			uint16_t param1, param2, param3;
		};
		std::vector<Instruction> ops;
		std::vector<std::size_t> labels;
		typedef std::size_t Label;
		int numRegisters;
		void UseRegister(int r) {
			if (r + 1 > numRegisters)
				numRegisters = r + 1;
		}
		Label DefineLabel() {
			auto s = labels.size();
			labels.push_back(static_cast<std::size_t>(-1));
			return s;
		}
		void MarkLabel(Label l) {
			SPAssert(labels[l] == static_cast<std::size_t>(-1));
			labels[l] = ops.size();
		}
		void Emit(OpCode op, uint16_t p1 = 0, uint16_t p2 = 0, uint16_t p3 = 0) {
			SPADES_MARK_FUNCTION_DEBUG();
			ops.push_back({op, p1, p2, p3});
			if (ops.size() > 30000) {
				SPRaise("Plural selector is too complex (instruction count limit exceeded)");
			}
		}

		template <class... T>
		void RaiseSyntaxErrorAtToken [[noreturn]] (StandardTokenizer::Iterator &it,
		                                          const std::string &format, T &&... args) {
			StandardTokenizer &tokenizer = it.GetTokenizer();
			const char *code = tokenizer.GetString();
			const char *loc = it.GetPointer();
			int len = static_cast<int>((*it).size());
			int codeLen = static_cast<int>(strlen(code));
			int index = static_cast<int>(loc - code);
			int trimStart = std::max(index - 20, 0);
			int trimEnd = std::min(index + len + 20, codeLen);
			std::string diagMessage = "  ";
			if (trimStart > 0) {
				diagMessage += "... ";
			}
			diagMessage += std::string(code).substr(trimStart, trimEnd - trimStart);
			if (trimEnd < codeLen) {
				diagMessage += " ...";
			}
			diagMessage += "\n  ";

			if (trimStart > 0) {
				diagMessage += "    ";
			}
			diagMessage += std::string(index - trimStart, ' ');
			diagMessage += std::string(len, '^');

			SPRaise(("Plural selector is invalid: " + format + "\n%s").c_str(),
			        std::forward<T>(args)..., diagMessage.c_str());
		}

		void ParseExpression(StandardTokenizer::Iterator &it, int outReg);

		void ParseValue(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			if (*it == "+") {
				++it;
				ParseValue(it, outReg);
			} else if (*it == "-") {
				++it;
				ParseValue(it, outReg);
				Emit(OpCode::Negate, outReg);
			} else if (*it == "!") {
				++it;
				ParseValue(it, outReg);
				Emit(OpCode::Not, outReg);
			} else if (*it == "~") {
				++it;
				ParseValue(it, outReg);
				Emit(OpCode::BitwiseNot, outReg);
			} else if (*it == "n") {
				++it;
				Emit(OpCode::LoadParam, outReg);
			} else if (*it == "(") {
				++it;
				ParseExpression(it, outReg);
				if (*it != ")") {
					RaiseSyntaxErrorAtToken(it, "open parenthesis");
				}
				++it;
			} else if ((*it).size() == 0) {
				RaiseSyntaxErrorAtToken(it, "unexpected EOF");
			} else {
				try {
					long v = std::stol(*it);
					Emit(OpCode::LoadConstant, outReg, static_cast<uint16_t>(v & 0xffff),
					     static_cast<uint16_t>(v >> 16));
					++it;
				} catch (...) {
					RaiseSyntaxErrorAtToken(it, "failed to parse literal '%s' as integer",
					                        (*it).c_str());
				}
			}
		}
		void ParseTerm(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseValue(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "*") {
					++it;
					ParseValue(it, outReg + 1);
					Emit(OpCode::Muliply, outReg, outReg + 1);
				} else if (s == "/") {
					++it;
					ParseValue(it, outReg + 1);
					Emit(OpCode::Divide, outReg, outReg + 1);
				} else if (s == "%") {
					++it;
					ParseValue(it, outReg + 1);
					Emit(OpCode::Modulus, outReg, outReg + 1);
				} else {
					break;
				}
			}
		}
		void ParseFours(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseTerm(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "+") {
					++it;
					ParseTerm(it, outReg + 1);
					Emit(OpCode::Add, outReg, outReg + 1);
				} else if (s == "-") {
					++it;
					ParseTerm(it, outReg + 1);
					Emit(OpCode::Add, outReg, outReg + 1);
				} else {
					break;
				}
			}
		}
		void ParseShifts(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseFours(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "<") {
					++it;
					if (*it == "<") {
						++it;
						ParseFours(it, outReg + 1);
						Emit(OpCode::ShiftLeft, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else if (s == ">") {
					++it;
					if (*it == ">") {
						++it;
						ParseFours(it, outReg + 1);
						Emit(OpCode::ShiftRight, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else {
					break;
				}
			}
		}
		void ParseComparsion(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseShifts(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "<") {
					++it;
					if (*it == "=") {
						++it;
						ParseShifts(it, outReg + 1);
						Emit(OpCode::LessOrEqual, outReg, outReg + 1);
					} else {
						ParseShifts(it, outReg + 1);
						Emit(OpCode::Less, outReg, outReg + 1);
					}
				} else if (s == ">") {
					++it;
					if (*it == "=") {
						++it;
						ParseShifts(it, outReg + 1);
						Emit(OpCode::GreaterOrEqual, outReg, outReg + 1);
					} else {
						ParseShifts(it, outReg + 1);
						Emit(OpCode::Greater, outReg, outReg + 1);
					}
				} else {
					break;
				}
			}
		}
		void ParseEquality(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseComparsion(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "=") {
					++it;
					if (*it == "=") {
						++it;
						ParseComparsion(it, outReg + 1);
						Emit(OpCode::Equality, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else if (s == "!") {
					++it;
					if (*it == "=") {
						++it;
						ParseComparsion(it, outReg + 1);
						Emit(OpCode::Inequality, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else {
					break;
				}
			}
		}
		void ParseBitwiseLogicalTerm(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseEquality(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "&") {
					++it;
					if (*it != "&") {
						ParseEquality(it, outReg + 1);
						Emit(OpCode::BitwiseAnd, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else {
					break;
				}
			}
		}
		void ParseBitwiseLogicalXor(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseBitwiseLogicalTerm(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "^") {
					++it;
					ParseBitwiseLogicalTerm(it, outReg + 1);
					Emit(OpCode::BitwiseXor, outReg, outReg + 1);
				} else {
					break;
				}
			}
		}
		void ParseBitwiseLogical(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseBitwiseLogicalXor(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "|") {
					++it;
					if (*it != "|") {
						ParseBitwiseLogicalXor(it, outReg + 1);
						Emit(OpCode::BitwiseOr, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else {
					break;
				}
			}
		}
		void ParseLogicalTerm(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseBitwiseLogical(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "&") {
					++it;
					if (*it == "&") {
						++it;
						ParseBitwiseLogical(it, outReg + 1);
						Emit(OpCode::And, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else {
					break;
				}
			}
		}

		void ParseLogical(StandardTokenizer::Iterator &it, int outReg) {
			SPADES_MARK_FUNCTION_DEBUG();
			UseRegister(outReg);
			ParseLogicalTerm(it, outReg);
			while (true) {
				auto s = *it;
				if (s == "|") {
					++it;
					if (*it == "|") {
						++it;
						ParseLogicalTerm(it, outReg + 1);
						Emit(OpCode::Or, outReg, outReg + 1);
					} else {
						--it;
						break;
					}
				} else {
					break;
				}
			}
		}

	public:
		PluralSelector(const char *expr) : numRegisters(0) {
			SPADES_MARK_FUNCTION();
			StandardTokenizer tokenizer(expr);
			auto it = tokenizer.begin();
			ParseExpression(it, 0);
			if (it != tokenizer.end()) {
				RaiseSyntaxErrorAtToken(it, "Plural selector is invalid: unexpected token: '%s'",
				                        (*it).c_str());
			}
		}

		int Execute(int value) {
			SPADES_MARK_FUNCTION();
			std::vector<int> reg;
			reg.resize(numRegisters);

#if DEBUG_INTERPRETER
			SPLog("Evaluating plural selector with n=%d", value);
#endif

			std::size_t ip = 0;
			while (ip < ops.size()) {
				auto &op = ops[ip];
#if DEBUG_INTERPRETER
				switch (op.op) {
					case OpCode::Jump: SPLog("  Jump to %d", (int)labels[op.param1]); break;
					case OpCode::JumpIfZero:
						SPLog("  Jump to %d if reg[%d] (%d) == 0", (int)labels[op.param2],
						      (int)op.param1, reg[op.param1]);
						break;
					case OpCode::Add:
						SPLog("  reg[%d] <- reg[%d] (%d) + reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Subtract:
						SPLog("  reg[%d] <- reg[%d] (%d) - reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Muliply:
						SPLog("  reg[%d] <- reg[%d] (%d) * reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Divide:
						SPLog("  reg[%d] <- reg[%d] (%d) / reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Modulus:
						SPLog("  reg[%d] <- reg[%d] (%d) % reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::ShiftLeft:
						SPLog("  reg[%d] <- reg[%d] (%d) << reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::ShiftRight:
						SPLog("  reg[%d] <- reg[%d] (%d) >> reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::BitwiseAnd:
						SPLog("  reg[%d] <- reg[%d] (%d) & reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::BitwiseOr:
						SPLog("  reg[%d] <- reg[%d] (%d) | reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::BitwiseXor:
						SPLog("  reg[%d] <- reg[%d] (%d) ^ reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::BitwiseNot:
						SPLog("  reg[%d] <- ~ reg[%d] (%d)", (int)op.param1, (int)op.param1,
						      reg[op.param1]);
						break;
					case OpCode::And:
						SPLog("  reg[%d] <- reg[%d] (%d) && reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Or:
						SPLog("  reg[%d] <- reg[%d] (%d) || reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Not:
						SPLog("  reg[%d] <- ! reg[%d] (%d)", (int)op.param1, (int)op.param1,
						      reg[op.param1]);
						break;
					case OpCode::Negate:
						SPLog("  reg[%d] <- - reg[%d] (%d)", (int)op.param1, (int)op.param1,
						      reg[op.param1]);
						break;
					case OpCode::Less:
						SPLog("  reg[%d] <- reg[%d] (%d) < reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::LessOrEqual:
						SPLog("  reg[%d] <- reg[%d] (%d) <= reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Greater:
						SPLog("  reg[%d] <- reg[%d] (%d) > reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::GreaterOrEqual:
						SPLog("  reg[%d] <- reg[%d] (%d) >= reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Equality:
						SPLog("  reg[%d] <- reg[%d] (%d) == reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::Inequality:
						SPLog("  reg[%d] <- reg[%d] (%d) != reg[%d] (%d)", (int)op.param1,
						      (int)op.param1, reg[op.param1], (int)op.param2, reg[op.param2]);
						break;
					case OpCode::LoadConstant:
						SPLog("  reg[%d] <- %d", (int)op.param1,
						      int(op.param2) | (int(op.param3) << 16));
						break;
					case OpCode::LoadParam:
						SPLog("  reg[%d] <- n (%d)", (int)op.param1, value);
						break;
				}
#endif
				switch (op.op) {
					case OpCode::Jump: ip = labels[op.param1]; break;
					case OpCode::JumpIfZero:
						if (reg[op.param1]) {
							ip++;
						} else {
							ip = labels[op.param2];
						}
						break;
					case OpCode::Add:
						reg[op.param1] = reg[op.param1] + reg[op.param2];
						ip++;
						break;
					case OpCode::Subtract:
						reg[op.param1] = reg[op.param1] - reg[op.param2];
						ip++;
						break;
					case OpCode::Muliply:
						reg[op.param1] = reg[op.param1] * reg[op.param2];
						ip++;
						break;
					case OpCode::Divide:
						if (reg[op.param2] == 0)
							SPRaise("Error while executing plural selector: division by zero");
						reg[op.param1] = reg[op.param1] / reg[op.param2];
						ip++;
						break;
					case OpCode::Modulus:
						if (reg[op.param2] == 0)
							SPRaise("Error while executing plural selector: division by zero");
						reg[op.param1] = reg[op.param1] % reg[op.param2];
						ip++;
						break;
					case OpCode::ShiftLeft:
						reg[op.param1] = reg[op.param1] << reg[op.param2];
						ip++;
						break;
					case OpCode::ShiftRight:
						reg[op.param1] = reg[op.param1] >> reg[op.param2];
						ip++;
						break;
					case OpCode::BitwiseAnd:
						reg[op.param1] = reg[op.param1] & reg[op.param2];
						ip++;
						break;
					case OpCode::BitwiseOr:
						reg[op.param1] = reg[op.param1] | reg[op.param2];
						ip++;
						break;
					case OpCode::BitwiseXor:
						reg[op.param1] = reg[op.param1] ^ reg[op.param2];
						ip++;
						break;
					case OpCode::BitwiseNot:
						reg[op.param1] = !reg[op.param1];
						ip++;
						break;
					case OpCode::And:
						reg[op.param1] = reg[op.param1] && reg[op.param2];
						ip++;
						break;
					case OpCode::Or:
						reg[op.param1] = reg[op.param1] || reg[op.param2];
						ip++;
						break;
					case OpCode::Not:
						reg[op.param1] = ~reg[op.param1];
						ip++;
						break;
					case OpCode::Negate:
						reg[op.param1] = -reg[op.param1];
						ip++;
						break;
					case OpCode::Less:
						reg[op.param1] = reg[op.param1] < reg[op.param2];
						ip++;
						break;
					case OpCode::LessOrEqual:
						reg[op.param1] = reg[op.param1] <= reg[op.param2];
						ip++;
						break;
					case OpCode::Greater:
						reg[op.param1] = reg[op.param1] > reg[op.param2];
						ip++;
						break;
					case OpCode::GreaterOrEqual:
						reg[op.param1] = reg[op.param1] >= reg[op.param2];
						ip++;
						break;
					case OpCode::Equality:
						reg[op.param1] = reg[op.param1] == reg[op.param2];
						ip++;
						break;
					case OpCode::Inequality:
						reg[op.param1] = reg[op.param1] != reg[op.param2];
						ip++;
						break;
					case OpCode::LoadConstant:
						reg[op.param1] = int(op.param2) | (int(op.param3) << 16);
						ip++;
						break;
					case OpCode::LoadParam:
						reg[op.param1] = value;
						ip++;
						break;
				}
			}
			return reg[0];
		}
	};

	void PluralSelector::ParseExpression(StandardTokenizer::Iterator &it, int outReg) {
		SPADES_MARK_FUNCTION();
		UseRegister(outReg);
		ParseLogical(it, outReg);

		if (*it == "?") {
			++it;

			Label falseLabel = DefineLabel();
			Label endLabel = DefineLabel();
			Emit(OpCode::JumpIfZero, static_cast<uint16_t>(outReg),
			     static_cast<uint16_t>(falseLabel));

			ParseExpression(it, outReg);
			Emit(OpCode::Jump, static_cast<uint16_t>(endLabel));

			if (*it != ":") {
				RaiseSyntaxErrorAtToken(it, "Unexpected token '%s': ':' expected", (*it).c_str());
			}
			++it;

			MarkLabel(falseLabel);
			ParseExpression(it, outReg);

			MarkLabel(endLabel);
		}
	}

	class CatalogOfLanguage {

		struct CatalogEntryKey {
			std::string text;
			int pluralId;

			struct Hash {
				std::size_t operator()(const CatalogEntryKey &ent) const {
					return std::hash<std::string>()(ent.text) ^ std::hash<int>()(ent.pluralId);
				}
			};

			bool operator==(const CatalogEntryKey &k) const {
				return text == k.text && pluralId == k.pluralId;
			}
		};

		std::unordered_map<CatalogEntryKey, std::string, CatalogEntryKey::Hash> entries;

		std::unique_ptr<PluralSelector> selector;

		class CatalogParser {
			const std::string &po;
			CatalogOfLanguage &catalog;
			std::size_t pos;
			int line;
			char newLineChar;
			void FoundNewLine(char c) {
				if (newLineChar == 0)
					newLineChar = c;
				if (c == newLineChar)
					line++;
			}
			void SkipWhitespace() {
				SPADES_MARK_FUNCTION();
				while (pos < po.size()) {
					switch (po[pos]) {
						case '\n':
						case '\r':
							FoundNewLine(po[pos]);
						// fall-through
						case ' ':
						case '\t': pos++; break;
						case '#':
							while (pos < po.size()) {
								if (po[pos] == '\n' || po[pos] == '\r') {
									FoundNewLine(po[pos]);
									pos++;
									break;
								}
								pos++;
							}
							break;
						default: return;
					}
				}
			}
			std::string ReadSymbol() {
				SPADES_MARK_FUNCTION();
				auto isSymbolChar = [](char c) {
					return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
					       (c >= '0' && c <= '9') || c == '_';
				};
				auto start = pos;
				auto p2 = pos;
				while (p2 < po.size()) {
					if (!isSymbolChar(po[p2]))
						break;
					p2++;
				}
				pos = p2;
				return po.substr(start, p2 - start);
			}
			std::string ReadString() {
				SPADES_MARK_FUNCTION();
				SPAssert(po[pos] == '"');
				pos++;

				std::string ret;
				while (true) {
					if (pos >= po.size()) {
						SPRaise("Unexpected EOF while parsing string at line %d", line);
					}
					char c = po[pos];
					if (c == '\\' && pos + 1 < po.size()) {
						char escapedChar = 0;
						switch (po[pos + 1]) {
							case 'n': escapedChar = '\n'; break;
							case 't': escapedChar = '\t'; break;
							case 'r': escapedChar = '\r'; break;
							case '"': escapedChar = '"'; break;
							case '\\': escapedChar = '\\'; break;
						}
						if (escapedChar != 0) {
							pos += 2;
							ret += escapedChar;
							continue;
						}
					} else if (c == '"') {
						pos++;
						return ret;
					} else if (c == '\n' || c == '\r') {
						SPRaise("Unexpected newline at line %d", line);
					}
					ret += c;
					pos++;
				}
				return ret;
			}
			std::string ReadIndexer() {
				SPADES_MARK_FUNCTION();
				pos++;
				auto start = pos;
				auto p2 = pos;
				while (true) {
					if (p2 >= po.size()) {
						SPRaise("Unexpected EOF at line %d", line);
					}
					if (po[p2] == ']')
						break;
					p2++;
				}
				pos = p2 + 1;
				return po.substr(start, p2 - start);
			}
			enum class TokenType { Symbol, String, Indexer };
			std::pair<TokenType, std::string> ReadToken() {
				SPADES_MARK_FUNCTION();
				if (pos >= po.size())
					SPRaise("Unexpected EOF at line %d", line);
				char c = po[pos];
				if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
				    c == '_') {
					return std::make_pair(TokenType::Symbol, ReadSymbol());
				} else if (c == '"') {
					return std::make_pair(TokenType::String, ReadString());
				} else if (c == '[') {
					return std::make_pair(TokenType::Indexer, ReadIndexer());
				} else {
					SPRaise("Unexpected character 0x%02x at line %d", c, line);
				}
			}
			void UnexpectedToken() {
				SPADES_MARK_FUNCTION();
				SPRaise("Unexpected token at line %d", line);
			}
			void ExpectToken(TokenType t, TokenType expect) {
				SPADES_MARK_FUNCTION();
				if (t != expect) {
					const char *s = nullptr;
					switch (expect) {
						case TokenType::Symbol: s = "Symbol"; break;
						case TokenType::String: s = "String"; break;
						case TokenType::Indexer: s = "Indexer"; break;
					}
					SPRaise("Unexpected token at line %d (expected '%s')", line, s);
				}
			}

		public:
			CatalogParser(const std::string &po, CatalogOfLanguage &catalog)
			    : po(po), catalog(catalog), pos(0), line(1), newLineChar(0) {}
			std::string header;
			void Parse() {
				SPADES_MARK_FUNCTION();
				bool readingMessage = false;
				std::string currentMsgId;
				std::string currentMsgIdPlural;
				std::string currentMsgCtxt = "$";
				std::map<int, std::string> currentMsgText;

				auto addCurrentMessage = [&]() {
					if (!readingMessage)
						return;
					if (currentMsgId.empty()) {
						header = currentMsgText[0];
					} else if (!currentMsgText[0].empty()) {
						currentMsgId = currentMsgCtxt + "\x01" + currentMsgId;
						for (const auto &pair : currentMsgText) {
							catalog.Add(currentMsgId, pair.first, pair.second);
						}
					}
					currentMsgId.clear();
					currentMsgIdPlural.clear();
					currentMsgText.clear();
					currentMsgCtxt = "$";
					readingMessage = false;
				};

				SkipWhitespace();
				std::string directive;
				int directiveIdx = 0;
				while (pos < po.size()) {
					auto tk = ReadToken();
					if (tk.first == TokenType::Symbol) {
						directive = tk.second;
						if (directive == "msgid") {
							if (currentMsgId.size() > 0)
								addCurrentMessage();
							readingMessage = true;
							currentMsgId = std::string();
						} else if (directive == "msgid_plural") {
							currentMsgIdPlural = std::string();
						} else if (directive == "msgctxt") {
							addCurrentMessage();
							currentMsgCtxt = std::string();
						} else if (directive == "msgstr") {
							directiveIdx = 0;
							SkipWhitespace();
							tk = ReadToken();
							if (tk.first == TokenType::Indexer) {
								try {
									directiveIdx = static_cast<int>(std::stol(tk.second));
									currentMsgText[directiveIdx] = std::string();
								} catch (...) {
									SPRaise("Integer parse error of '%s' at line %d",
									        tk.second.c_str(), line);
								}
							} else {
								currentMsgText[0] = std::string();
								goto readString;
							}
						} else {
							SPRaise("Unknown directive '%s'", directive.c_str());
						}
					} else {
					readString:
						ExpectToken(tk.first, TokenType::String);
						if (directive == "msgid") {
							currentMsgId += std::move(tk.second);
						} else if (directive == "msgid_plural") {
							currentMsgIdPlural += std::move(tk.second);
						} else if (directive == "msgctxt") {
							currentMsgCtxt += std::move(tk.second);
						} else if (directive == "msgstr") {
							int idx = directiveIdx;
							auto s = std::move(currentMsgText[idx]);
							s += tk.second;
							currentMsgText[idx] = std::move(s);
						}
					}
					SkipWhitespace();
				}

				addCurrentMessage();
			}
		};

	public:
		CatalogOfLanguage(const std::string &po) {
			SPADES_MARK_FUNCTION();
			CatalogParser parser(po, *this);
			parser.Parse();

			auto hdr = SplitIntoLines(parser.header);
			for (const auto &line : hdr) {
				auto pos = line.find(':');
				if (pos == std::string::npos)
					continue;
				auto key = line.substr(0, pos);
				key = TrimSpaces(key);
				if (EqualsIgnoringCase(key, "Plural-Forms")) {
					auto val = line.substr(pos + 1);
					if (selector != nullptr) {
						SPRaise("Multiple Plural-Forms found.");
					}

					auto itms = Split(val, ";");
					for (const auto &itm : itms) {
						pos = itm.find('=');
						if (pos == std::string::npos)
							continue;
						key = TrimSpaces(itm.substr(0, pos));
						if (EqualsIgnoringCase(key, "plural")) {
							val = itm.substr(pos + 1);
							selector = decltype(selector)(new PluralSelector(val.c_str()));
						}
					}
				}
			}
		}

		std::pair<std::string, bool> Find(const std::string &key, int num) {
			SPADES_MARK_FUNCTION();
			if (entries.empty())
				return std::make_pair(std::string(), false);
			int pl = selector != nullptr ? selector->Execute(num) : 0;
			CatalogEntryKey k = {key, pl};
			auto it = entries.find(k);
			if (it == entries.end()) {
				k.pluralId = 0;
				it = entries.find(k);
			}
			if (it == entries.end()) {
				return std::make_pair(std::string(), false);
			} else {
				return std::make_pair(it->second, true);
			}
		}

		void Add(const std::string &key, int pluralId, const std::string &text) {
			SPADES_MARK_FUNCTION_DEBUG();
			entries[{key, pluralId}] = text;
		}
	};

	class CatalogDomain {
		std::string domainName;
		std::unordered_map<std::string, std::shared_ptr<CatalogOfLanguage>> langs;

	public:
		CatalogDomain(const std::string &name) : domainName(name) {}

		std::shared_ptr<CatalogOfLanguage> operator[](const std::string &s) {
			SPADES_MARK_FUNCTION();
			auto it = langs.find(s);
			if (it == langs.end()) {
				std::shared_ptr<CatalogOfLanguage> c;
				if (!s.empty()) {
					std::string path = "Locales/" + s + "/" + domainName + ".po";
					if (FileManager::FileExists(path.c_str())) {
						try {
							c.reset(new CatalogOfLanguage(FileManager::ReadAllBytes(path.c_str())));
							SPLog("Catalog file '%s' loaded", path.c_str());
						} catch (const std::exception &ex) {
							// c will be left unset
							SPLog("Failed to load the catalog file '%s': %s", path.c_str(),
							      ex.what());
						}
					} else {
						SPLog("Catalog file '%s' not found (locale='%s', domain='%s')",
						      path.c_str(), s.c_str(), domainName.c_str());
					}
				}
				langs[s] = c;
				return c;
			}
			return it->second;
		}
	};

	namespace {
		std::unordered_map<std::string, std::shared_ptr<CatalogDomain>> domains;
		std::unordered_map<std::string, std::shared_ptr<CatalogOfLanguage>> langCatalogCache;

		std::string currentLocaleRegion;
		std::string currentLocale;
		std::string lastLocale;

		class CatalogCacheInvalidator : public ISettingItemListener {
		public:
			void SettingChanged(const std::string &) override { LoadCurrentLocale(); }
		};

		std::unique_ptr<CatalogCacheInvalidator> invalidator;
	}

	void LoadCurrentLocale() {
		SPADES_MARK_FUNCTION();

		if (!invalidator) {
			invalidator.reset(new CatalogCacheInvalidator());
			core_locale.AddListener(invalidator.get());
		}

		std::string locale = core_locale;
		if (locale == lastLocale)
			return;

		// Locale was changed; reload catalogs
		lastLocale = locale;
		langCatalogCache.clear();

		if (locale.size() == 0) {
			// check user locale
			locale = GetUserLocale();
			auto p = locale.find('.');
			if (p != std::string::npos)
				locale = locale.substr(0, p);
		}

		// Normalize
		for (auto &c : locale) {
			c = tolower(c);
			if (c == '-') {
				c = '_';
			}
		}

		if (locale == "en_us") {
			locale.clear();
		}

		auto p = std::min(locale.find('_'), locale.find('-'));
		if (p != std::string::npos) {
			// The separator must be an underscore
			locale[p] = '_';
		}

		currentLocaleRegion = locale;

		if (p != std::string::npos)
			locale = locale.substr(0, p);
		currentLocale = locale;
	}

	std::string GetCurrentLocaleAndRegion() {
		LoadCurrentLocale();
		if (currentLocaleRegion.empty()) {
			return "en_us";
		}
		return currentLocaleRegion;
	}

	static std::shared_ptr<CatalogDomain> GetDomain(const std::string &s) {
		SPADES_MARK_FUNCTION();

		auto it = domains.find(s);
		if (it == domains.end()) {
			std::shared_ptr<CatalogDomain> c(new CatalogDomain(s));
			domains[s] = c;
			return c;
		}
		return it->second;
	}

	static std::shared_ptr<CatalogOfLanguage> GetCatalogOfLanguage(const std::string &domainName) {
		SPADES_MARK_FUNCTION();

		auto it = langCatalogCache.find(domainName);
		if (it == langCatalogCache.end()) {
			auto domain = GetDomain(domainName);
			auto cat = (*domain)[currentLocaleRegion];
			if (cat == nullptr) {
				cat = (*domain)[currentLocale];
			}
			if (cat == nullptr) {
				if (!currentLocaleRegion.empty()) {
					SPLog("Catalog file for the locale '%s' was not found. (domain='%s')",
					      currentLocaleRegion.c_str(), domainName.c_str());
				}
				SPLog("Using the default locale. (domain='%s')", domainName.c_str());
			}
			langCatalogCache[domainName] = cat;
			return cat;
		}
		return it->second;
	}

	std::string GetTextRaw(const std::string &domain, const std::string &ctx,
	                       const std::string &text, int plural) {
		SPADES_MARK_FUNCTION();

		auto cat = GetCatalogOfLanguage(domain);
		if (cat == nullptr)
			return text;
		auto e = cat->Find(ctx + "\x01" + text, plural);
		if (e.second) {
			return e.first;
		} else {
			return text;
		}
	}

	std::string GetTextRawPlural(const std::string &domain, const std::string &ctx,
	                             const std::string &text, const std::string &textPlural,
	                             int plural) {
		SPADES_MARK_FUNCTION();

		auto cat = GetCatalogOfLanguage(domain);
		if (cat == nullptr)
			return plural == 1 ? text : textPlural;
		auto e = cat->Find(ctx + "\x01" + text, plural);
		if (e.second) {
			return e.first;
		} else {
			return plural == 1 ? text : textPlural;
		}
	}

	CatalogDomainHandle defaultDomain("openspades");

	CatalogDomainHandle::CatalogDomainHandle(const std::string &domain) : domain(domain) {}
}
