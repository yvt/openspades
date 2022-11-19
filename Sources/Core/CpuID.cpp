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
#elif defined(__i386__) && (defined(__pic__) || defined(__PIC__))
        asm volatile("mov %%ebx, %%edi\ncpuid\nxchg %%edi, %%ebx\n"
                     : "=a"(regs[0]), "=D"(regs[1]), "=c"(regs[2]), "=d"(regs[2])
		             : "a"(a), "c"(0));
#else
		asm volatile("cpuid"
		             : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
		             : "a"(a), "c"(0));
#endif
		return regs;
	}

	static uint32_t xcr0() {
#ifdef WIN32
		return static_cast<uint32_t>(_xgetbv(0));
#else
		uint32_t a;
		asm volatile("xgetbv" : "=a"(a) : "c"(0) : "%edx");
		return a;
#endif
	}

	CpuID::CpuID() {
		uint32_t maxStdLevel;
		{
			auto ar = cpuid(0);
			char buf[13];
			maxStdLevel = ar[0];
			memcpy(&buf[0], &ar[1], 4);
			memcpy(&buf[4], &ar[3], 4);
			memcpy(&buf[8], &ar[2], 4);
			buf[12] = 0;
			vendor = buf;
		}
		{
			auto ar = cpuid(1);
			featureEcx = ar[2];
			featureEdx = ar[3];

			// xsave/osxsave
			if ((featureEcx & (1U << 28)) && (featureEcx & 26) && (featureEcx & 27)) {
				auto x = xcr0();
				featureXcr0Avx = ((x & 6) == 6);
				featureXcr0Avx512 = ((x & 224) == 224);
			}
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
			case CpuFeature::AVX: return featureXcr0Avx;
			case CpuFeature::AVX2: return (featureXcr0Avx && subfeature & (1U << 5));
			case CpuFeature::AVX512CD: return (featureXcr0Avx512 && subfeature & (1U << 28));
			case CpuFeature::AVX512ER: return (featureXcr0Avx512 && subfeature & (1U << 27));
			case CpuFeature::AVX512PF: return (featureXcr0Avx512 && subfeature & (1U << 26));
			case CpuFeature::AVX512F: return (featureXcr0Avx512 && subfeature & (1U << 16));
			case CpuFeature::SimultaneousMT: return featureEdx & (1U << 28);
		}
	}

#endif
}
