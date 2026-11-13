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
// Select a mutation so the verify check must fire: 1 = L2, 2 = L1, 3 = L3. 0 = none.
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
	return nreg == 12 && r0 == 0x02050102 && r1 == 0x01020501 && r2 == 0x05010205;
}

// Packed A+D addresses that must not enter the L1 fused loop.
// Colour (0x01), ST (0x02), UV (0x03) and fog (0x0a) are accepted: the loop
// runs the A+D handler first, then the packed ST, RGBA and XYZF2 stores, which
// read m_q, m_v and the latched UV at the same moment the per-register handlers would.
inline bool FusedL1AddrAllowed(u8 addr)
{
	switch (addr)
	{
		case 0x04: // XYZF2 kick
		case 0x05: // XYZ2 kick
		case 0x0c: // XYZF3 kick
		case 0x0d: // XYZ3 kick
		case 0x53: // starts a transfer
		case 0x54: // host write into local memory
			return false;
		default:
			return true;
	}
}

// Packed A+D addresses whose handlers write state a verify trial cannot restore.
// SIGNAL/FINISH/LABEL may raise interrupts or touch privileged registers.
// Packets that contain one of these are counted as skip_state and run live
// without a dual trial.
inline bool FusedL1AddrSkipState(u8 addr)
{
	switch (addr)
	{
		case 0x60: // SIGNAL
		case 0x61: // FINISH
		case 0x62: // LABEL
			return true;
		default:
			return false;
	}
}
