// WTFPL
#pragma once

#include <array>
#include <cstdint>
#include <string>

#if defined(__i386__) || defined(_M_IX86)
	// FIXME: Why does this preprocessor condition even exists?
#endif

namespace spades {

	enum class CpuFeature {
		MMX,
		SSE,
		SSE2,
		SSE3,
		SSSE3,
		FMA,
		AVX,
		AVX2,
		AVX512CD,
		AVX512ER,
		AVX512PF,
		AVX512F,
		SimultaneousMT
	};

#if defined(__i386__) || defined(_M_IX86) || defined(__amd64__) || defined(__x86_64__)

	class CpuID {
		std::string vendor;
		std::string brand;
		uint32_t featureEcx;
		uint32_t featureEdx;
		uint32_t subfeature;
		std::string info;

	public:
		CpuID();

		bool Supports(CpuFeature feature);

		const std::string &GetVendorId() { return vendor; }
		const std::string &GetBrand() { return brand; }

		const std::string &GetMiscInfo() { return info; }
	};
#else

	class CpuID {
	public:
		CpuID() {}
		bool Supports(CpuFeature feature) { return false; }

		std::string GetVendorId() { return "Unknown"; }
		std::string GetBrand() { return "Unknown"; }
		std::string GetMiscInfo() { return "(none)"; }
	};

#endif
}
