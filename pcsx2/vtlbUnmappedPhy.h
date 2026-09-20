// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "MemoryTypes.h"
#include "common/SingleRegisterTypes.h"

// Default handlers for physical pages with no overlay. Private to vtlb.cpp and
// the unmapped-physical unit test; not part of the public vtlb API.

extern mem8_t vtlbDefaultPhyRead8(u32 addr);
extern mem16_t vtlbDefaultPhyRead16(u32 addr);
extern mem32_t vtlbDefaultPhyRead32(u32 addr);
extern mem64_t vtlbDefaultPhyRead64(u32 addr);
extern RETURNS_R128 vtlbDefaultPhyRead128(u32 addr);
extern void vtlbDefaultPhyWrite8(u32 addr, mem8_t data);
extern void vtlbDefaultPhyWrite16(u32 addr, mem16_t data);
extern void vtlbDefaultPhyWrite32(u32 addr, mem32_t data);
extern void vtlbDefaultPhyWrite64(u32 addr, mem64_t data);
extern void TAKES_R128 vtlbDefaultPhyWrite128(u32 addr, r128 data);
extern void vtlbResetUnmappedPhysicalLog();
