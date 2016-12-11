// WTFPL

#include <cstring>

// Fixes #473
#ifdef WIN32
	#include <intrin.h>
#endif

#include "CpuID.h"

namespace spades {

#if defined(__i386__) || defined(_M_IX86) || defined(__amd64__) || defined(__x86_64__)
	static std::array<uint32_t, 4> cpuid(uint32_t a) {
		std::array<uint32_t, 4> regs;
#ifdef WIN32
		__cpuid(reinterpret_cast<int *>(regs.data()), a);
#else
		asm volatile("cpuid"
		             : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
		             : "a"(a), "c"(0));
#endif
		return regs;
	}

	CpuID::CpuID() {
		uint32_t maxStdLevel;
		{
			auto ar = cpuid(0);
			char buf[13];
			buf[12] = 0;
			maxStdLevel = ar[0];
			memcpy(buf, ar.data() + 1, 12);
			vendor = buf;
		}
		{
			auto ar = cpuid(1);
			featureEcx = ar[2];
			featureEdx = ar[3];
		}

		{
			if (cpuid(0x80000000U)[0] >= 0x80000004U) {
				char buf[49];
				buf[48] = 0;
				memcpy(buf, cpuid(0x80000002U).data(), 16);
				memcpy(buf + 16, cpuid(0x80000003U).data(), 16);
				memcpy(buf + 32, cpuid(0x80000004U).data(), 16);
				brand = buf;
			} else {
				brand = "Unknown";
			}
		}
		{
			auto ar = cpuid(7);
			// FIXME: sublevels?
			subfeature = ar[1];
		}
		{ info = "(none)"; }
	}

	bool CpuID::Supports(spades::CpuFeature feature) {
		switch (feature) {
			case CpuFeature::MMX: return featureEdx & (1U << 23);
			case CpuFeature::SSE: return featureEdx & (1U << 25);
			case CpuFeature::SSE2: return featureEdx & (1U << 26);
			case CpuFeature::SSE3: return featureEcx & (1U << 0);
			case CpuFeature::SSSE3: return featureEcx & (1U << 9);
			case CpuFeature::FMA: return featureEcx & (1U << 12);
			case CpuFeature::AVX: return featureEcx & (1U << 28);
			case CpuFeature::AVX2: return subfeature & (1U << 5);
			case CpuFeature::AVX512CD: return subfeature & (1U << 28);
			case CpuFeature::AVX512ER: return subfeature & (1U << 27);
			case CpuFeature::AVX512PF: return subfeature & (1U << 26);
			case CpuFeature::AVX512F: return subfeature & (1U << 16);
			case CpuFeature::SimultaneousMT: return featureEdx & (1U << 28);
		}
	}

#endif
}
