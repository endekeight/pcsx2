// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "common/Pcsx2Types.h"

// Fuse packed GIF layouts behind this switch. Ships 0.
// Defined only here, never on the compiler command line: GSState's layout depends on it,
// so every translation unit must see the same value.
#define GS_GIF_FUSED_LAYOUTS 0
// Dual-run oracle that the fused packed handlers match the per-quadword loop. Ships 0.
#define GS_GIF_FUSED_VERIFY 0
// Select a mutation so the verify check must fire: 1 = L2. 0 = none.
#define GS_GIF_FUSED_MUTATE 0

#if GS_GIF_FUSED_VERIFY && !GS_GIF_FUSED_LAYOUTS
#error GS_GIF_FUSED_VERIFY requires GS_GIF_FUSED_LAYOUTS
#endif
#if GS_GIF_FUSED_MUTATE && !GS_GIF_FUSED_VERIFY
#error GS_GIF_FUSED_MUTATE requires GS_GIF_FUSED_VERIFY
#endif
#if GS_GIF_FUSED_VERIFY && defined(NDEBUG)
#error GS_GIF_FUSED_VERIFY is not for a build with NDEBUG
#endif
#if GS_GIF_FUSED_MUTATE && defined(NDEBUG)
#error GS_GIF_FUSED_MUTATE is not for a build with NDEBUG
#endif

// Packed REGS after GIFPath::SetTag: one register per byte, register 0 in the lowest byte.
inline bool MatchPackedLayoutL1(u32 nreg, u32 r0, u32 r1, u32 r2)
{
	(void)r1;
	(void)r2;
	return nreg == 4 && r0 == 0x0401020e;
}

inline bool MatchPackedLayoutL2x4(u32 nreg, u32 r0, u32 r1, u32 r2)
{
	return nreg == 9 && r0 == 0x02040201 && r1 == 0x02040204 && r2 == 0x00000004;
}

inline bool MatchPackedLayoutL2x3(u32 nreg, u32 r0, u32 r1, u32 r2)
{
	return nreg == 7 && r0 == 0x02040201 && r1 == 0x00040204 && r2 == 0x00000000;
}

inline bool MatchPackedLayoutL3(u32 nreg, u32 r0, u32 r1, u32 r2)
{
	return nreg == 12 && r0 == 0x02050102 && r1 == 0x01020501 && r2 == 0x05020105;
}
