// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSGifFused.h"

#include <gtest/gtest.h>

namespace
{
	bool AnyMatch(u32 nreg, u32 r0, u32 r1, u32 r2)
	{
		return MatchPackedLayoutL1(nreg, r0, r1, r2) || MatchPackedLayoutL2x4(nreg, r0, r1, r2) ||
			   MatchPackedLayoutL2x3(nreg, r0, r1, r2) || MatchPackedLayoutL3(nreg, r0, r1, r2);
	}
} // namespace

TEST(GifFusedLayout, L1AcceptsOwnOnly)
{
	EXPECT_TRUE(MatchPackedLayoutL1(4, 0x0401020e, 0, 0));
	EXPECT_FALSE(MatchPackedLayoutL2x4(4, 0x0401020e, 0, 0));
	EXPECT_FALSE(MatchPackedLayoutL2x3(4, 0x0401020e, 0, 0));
	EXPECT_FALSE(MatchPackedLayoutL3(4, 0x0401020e, 0, 0));
}

TEST(GifFusedLayout, L2x4AcceptsOwnOnly)
{
	EXPECT_TRUE(MatchPackedLayoutL2x4(9, 0x02040201, 0x02040204, 0x00000004));
	EXPECT_FALSE(MatchPackedLayoutL1(9, 0x02040201, 0x02040204, 0x00000004));
	EXPECT_FALSE(MatchPackedLayoutL2x3(9, 0x02040201, 0x02040204, 0x00000004));
	EXPECT_FALSE(MatchPackedLayoutL3(9, 0x02040201, 0x02040204, 0x00000004));
}

TEST(GifFusedLayout, L2x3AcceptsOwnOnly)
{
	EXPECT_TRUE(MatchPackedLayoutL2x3(7, 0x02040201, 0x00040204, 0));
	EXPECT_FALSE(MatchPackedLayoutL1(7, 0x02040201, 0x00040204, 0));
	EXPECT_FALSE(MatchPackedLayoutL2x4(7, 0x02040201, 0x00040204, 0));
	EXPECT_FALSE(MatchPackedLayoutL3(7, 0x02040201, 0x00040204, 0));
}

TEST(GifFusedLayout, L3AcceptsOwnOnly)
{
	EXPECT_TRUE(MatchPackedLayoutL3(12, 0x02050102, 0x01020501, 0x05010205));
	EXPECT_FALSE(MatchPackedLayoutL1(12, 0x02050102, 0x01020501, 0x05010205));
	EXPECT_FALSE(MatchPackedLayoutL2x4(12, 0x02050102, 0x01020501, 0x05010205));
	EXPECT_FALSE(MatchPackedLayoutL2x3(12, 0x02050102, 0x01020501, 0x05010205));
}

TEST(GifFusedLayout, NineRegUpstreamFoldMatchesNone)
{
	EXPECT_FALSE(AnyMatch(9, 0x02040102, 0x01020401, 0x00000004));
}

TEST(GifFusedLayout, TwelveRegXYZF2FoldMatchesNone)
{
	EXPECT_FALSE(AnyMatch(12, 0x02040102, 0x01020401, 0x04010204));
}

TEST(GifFusedLayout, MatchingR0WrongNregMatchesNone)
{
	EXPECT_FALSE(AnyMatch(5, 0x0401020e, 0, 0));
	EXPECT_FALSE(AnyMatch(8, 0x02040201, 0x02040204, 0x00000004));
	EXPECT_FALSE(AnyMatch(9, 0x02040201, 0x00040204, 0));
	EXPECT_FALSE(AnyMatch(11, 0x02050102, 0x01020501, 0x05010205));
	EXPECT_FALSE(MatchPackedLayoutL1(9, 0x0401020e, 0, 0));
	EXPECT_FALSE(MatchPackedLayoutL2x4(7, 0x02040201, 0x02040204, 0x00000004));
	EXPECT_FALSE(MatchPackedLayoutL2x3(9, 0x02040201, 0x00040204, 0));
	EXPECT_FALSE(MatchPackedLayoutL3(9, 0x02050102, 0x01020501, 0x05010205));
}

TEST(GifFusedLayout, TwelveRegXYZ2SwappedSTRGBAMatchesNone)
{
	EXPECT_FALSE(AnyMatch(12, 0x02050102, 0x01020501, 0x05020105));
}

TEST(GifFusedLayout, L1AddrAllowed)
{
	EXPECT_TRUE(FusedL1AddrAllowed(0x01));
	EXPECT_TRUE(FusedL1AddrAllowed(0x02));
	EXPECT_TRUE(FusedL1AddrAllowed(0x03));
	EXPECT_FALSE(FusedL1AddrAllowed(0x04));
	EXPECT_FALSE(FusedL1AddrAllowed(0x05));
	EXPECT_TRUE(FusedL1AddrAllowed(0x0a));
	EXPECT_FALSE(FusedL1AddrAllowed(0x0c));
	EXPECT_FALSE(FusedL1AddrAllowed(0x0d));
	EXPECT_FALSE(FusedL1AddrAllowed(0x53));
	EXPECT_FALSE(FusedL1AddrAllowed(0x54));
	EXPECT_TRUE(FusedL1AddrAllowed(0x00));
	EXPECT_TRUE(FusedL1AddrAllowed(0x06));
	EXPECT_TRUE(FusedL1AddrAllowed(0x07));
	EXPECT_TRUE(FusedL1AddrAllowed(0x08));
	EXPECT_TRUE(FusedL1AddrAllowed(0x0f));
	EXPECT_TRUE(FusedL1AddrAllowed(0x1a));
	EXPECT_TRUE(FusedL1AddrAllowed(0x1b));
	EXPECT_TRUE(FusedL1AddrAllowed(0x3f));
	EXPECT_TRUE(FusedL1AddrAllowed(0x50));
	EXPECT_TRUE(FusedL1AddrAllowed(0x51));
	EXPECT_TRUE(FusedL1AddrAllowed(0x52));
}
