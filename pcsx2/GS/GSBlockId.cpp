// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSBlockId.h"

#if defined(ARCH_ARM64)
#include <arm_neon.h>
#if defined(__linux__)
#include <sys/auxv.h>
#include <asm/hwcap.h>
#endif
#endif

#include "GS/GSXXH.h"

#include <cstring>

namespace GSBlockId
{
	static_assert(GS_BLOCK_SIZE == 256u);

	u64 HashPortable(const u8* block)
	{
		// XXH3_64bits is the algorithm behind GSXXH3_64bits. The wrapper's long
		// path needs MultiISAFunctions, which the host test binary does not link.
		return XXH3_64bits(block, GS_BLOCK_SIZE);
	}

#if defined(ARCH_ARM64)

#if defined(__GNUC__)
#define GS_BLOCK_ID_TARGET_AES __attribute__((target("aes"), noinline))
#else
#define GS_BLOCK_ID_TARGET_AES
#endif

	GS_BLOCK_ID_TARGET_AES
	u64 HashAES(const u8* block)
	{
		// Four independent 128-bit lanes. Each lane absorbs a 16-byte stripe
		// every 64 bytes (four AES rounds cover the 256-byte block). Distinct
		// IVs stop identical stripes from cancelling across lanes.
		static const u64 iv[8] = {
			0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL,
			0xA4093822299F31D0ULL, 0x082EFA98EC4E6C89ULL,
			0x452821E638D01377ULL, 0xBE5466CF34E90C6CULL,
			0xC0AC29B7C97C50DDULL, 0x3F84D5B5B5470917ULL,
		};
		uint8x16_t a0 = vreinterpretq_u8_u64(vld1q_u64(iv + 0));
		uint8x16_t a1 = vreinterpretq_u8_u64(vld1q_u64(iv + 2));
		uint8x16_t a2 = vreinterpretq_u8_u64(vld1q_u64(iv + 4));
		uint8x16_t a3 = vreinterpretq_u8_u64(vld1q_u64(iv + 6));
		for (u32 round = 0; round < 4; round++)
		{
			const u8* p = block + round * 64;
			a0 = vaesmcq_u8(vaeseq_u8(a0, vld1q_u8(p + 0)));
			a1 = vaesmcq_u8(vaeseq_u8(a1, vld1q_u8(p + 16)));
			a2 = vaesmcq_u8(vaeseq_u8(a2, vld1q_u8(p + 32)));
			a3 = vaesmcq_u8(vaeseq_u8(a3, vld1q_u8(p + 48)));
		}
		// Cross-lane AES rounds so every stripe reaches the folded state, then
		// a multiply-xorshift so the 64-bit result is not a plain XOR of lanes.
		uint8x16_t f = vaesmcq_u8(vaeseq_u8(a0, a1));
		f = vaesmcq_u8(vaeseq_u8(f, a2));
		f = vaeseq_u8(f, a3);
		const uint64x2_t t = vreinterpretq_u64_u8(f);
		const u64 lo = vgetq_lane_u64(t, 0);
		const u64 hi = vgetq_lane_u64(t, 1);
		return Mix64(lo + hi * 0x9E3779B97F4A7C15ULL);
	}

	static bool CpuHasAES()
	{
#if defined(__ARM_FEATURE_AES)
		return true;
#elif defined(__APPLE__)
		return true;
#elif defined(__linux__)
		return (getauxval(AT_HWCAP) & HWCAP_AES) != 0;
#else
		return false;
#endif
	}

#endif // ARCH_ARM64

	struct Ops
	{
		u64 (*hash)(const u8*);
		const char* name;
	};

	static const Ops kPortableOps = {HashPortable, "portable"};
#if defined(ARCH_ARM64)
	static const Ops kAESOps = {HashAES, "aes"};
#endif

	static const Ops* SelectOps()
	{
#if defined(ARCH_ARM64)
		if (CpuHasAES())
			return &kAESOps;
#endif
		return &kPortableOps;
	}

	static const Ops* const s_ops = SelectOps();

	u64 Hash(const u8* block)
	{
		return s_ops->hash(block);
	}

	const char* ImplName()
	{
		return s_ops->name;
	}
} // namespace GSBlockId
