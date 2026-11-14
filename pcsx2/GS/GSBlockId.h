// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/GSRegs.h"

#include <cstddef>

namespace GSBlockId
{
	// Starting value for a Combine loop over block identities.
	inline constexpr u64 kCombineInit = 0x9E3779B97F4A7C15ULL;

	// Identity of exactly GS_BLOCK_SIZE (256) bytes. `block` must be 16-byte aligned.
	u64 Hash(const u8* block);

	// Identity of an arbitrary byte run. Any alignment. Length is mixed in.
	u64 HashBytes(const u8* data, size_t size);

	// Order-sensitive fold of one more identity into an accumulator.
	u64 Combine(u64 acc, u64 id);

	// "aes", "crc", or "portable".
	const char* ImplName();

	// exposed for tests
	u64 HashPortable(const u8* block);
#if defined(ARCH_ARM64)
	u64 HashAES(const u8* block);
	u64 HashCRC(const u8* block);
#endif
} // namespace GSBlockId
