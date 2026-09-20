// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

// The host harness cannot call vtlb_Init: that needs vtlb_Core_Alloc (SysMemory,
// a 4 GB fastmem area, and a page-fault handler) and vtlb_memRead walks vmap.
// Compiling the full vtlb.cpp translation unit is the same problem — it pulls
// Cache, COP0, R5900 TLB, VMManager, and SharedMemoryMappingArea, and it
// duplicates vtlbdata already provided by TestStubs.cpp. The harness therefore
// compiles pcsx2/vtlbUnmappedPhy.cpp directly, the way it already compiles
// pcsx2/Patch.cpp, with -DPCSX2_DEBUG -DPCSX2_DEVBUILD so pxFail is live.
// CMake core_test links libpcsx2 and does not compile this file separately.

#include "vtlbUnmappedPhy.h"

#include <gtest/gtest.h>

TEST(VtlbUnmappedPhy, Read32ReturnsZeroAndDoesNotAbort)
{
	EXPECT_EQ(vtlbDefaultPhyRead32(0x16409100), 0u);
	EXPECT_EQ(vtlbDefaultPhyRead32(0x10014D00), 0u);
}

TEST(VtlbUnmappedPhy, Write32IsDiscarded)
{
	const u32 canary = 0x12345678;
	vtlbDefaultPhyWrite32(0x16409100, 0xDEADBEEF);
	EXPECT_EQ(canary, 0x12345678u);
	EXPECT_EQ(vtlbDefaultPhyRead32(0x16409100), 0u);
}

TEST(VtlbUnmappedPhy, RepeatedAccessDoesNotAbort)
{
	for (int i = 0; i < 8; i++)
		EXPECT_EQ(vtlbDefaultPhyRead32(0x16409100), 0u);

	for (int i = 0; i < 8; i++)
		vtlbDefaultPhyWrite32(0x16409100, static_cast<mem32_t>(i));

	EXPECT_EQ(vtlbDefaultPhyRead32(0x16409100), 0u);
}

TEST(VtlbUnmappedPhy, OtherWidthsReturnZero)
{
	EXPECT_EQ(vtlbDefaultPhyRead8(0x16409100), 0);
	EXPECT_EQ(vtlbDefaultPhyRead16(0x16409100), 0);
	EXPECT_EQ(vtlbDefaultPhyRead64(0x16409100), 0u);
	vtlbDefaultPhyWrite8(0x16409100, 0xFF);
	vtlbDefaultPhyWrite16(0x16409100, 0xFFFF);
	vtlbDefaultPhyWrite64(0x16409100, 0xFFFFFFFFFFFFFFFFULL);
	EXPECT_EQ(vtlbDefaultPhyRead8(0x16409100), 0);
	EXPECT_EQ(vtlbDefaultPhyRead16(0x16409100), 0);
	EXPECT_EQ(vtlbDefaultPhyRead64(0x16409100), 0u);
}

TEST(VtlbUnmappedPhy, BoundedTableDoesNotAbort)
{
	for (u32 i = 0; i < 80; i++)
		EXPECT_EQ(vtlbDefaultPhyRead32(0x16000000u + (i * 0x1000u)), 0u);
}
