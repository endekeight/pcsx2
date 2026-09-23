// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSRegs.h"

#include <gtest/gtest.h>

namespace
{
	// Models a PSMCT32 texture (8x8-texel blocks): returns the largest bitmap row
	// (GSTextureBlockIndex >> 5) touched by any block of the texture.
	u32 MaxRow(u32 tw_log2, u32 th_log2)
	{
		const u32 width_blocks = (1u << tw_log2) >> 3;
		const u32 height_blocks = (1u << th_log2) >> 3;

		u32 max_row = 0;
		for (u32 blkY = 0; blkY < height_blocks; blkY++)
		{
			for (u32 blkX = 0; blkX < width_blocks; blkX++)
			{
				const u32 row = GSTextureBlockIndex(blkX, blkY) >> 5;
				if (row > max_row)
					max_row = row;
			}
		}
		return max_row;
	}
} // namespace

TEST(GSTextureBlockIndex, RowsStayInBitmapForWideTexture)
{
	// TW=11: unwrapped this reaches row 515.
	EXPECT_LT(MaxRow(11, 10), GS_MAX_PAGES);
}

TEST(GSTextureBlockIndex, RowsStayInBitmapForTallTexture)
{
	// TH=11: unwrapped this reaches row 1023.
	EXPECT_LT(MaxRow(10, 11), GS_MAX_PAGES);
}

TEST(GSTextureBlockIndex, RowsStayInBitmapForLargestTexture)
{
	EXPECT_LT(MaxRow(11, 11), GS_MAX_PAGES);
}

TEST(GSTextureBlockIndex, MatchesUnwrappedIndexUpTo1024Texels)
{
	// Textures up to 1024x1024 are unaffected; count mismatches so a failure
	// does not print 16384 lines.
	u32 mismatches = 0;
	for (u32 blkY = 0; blkY < 128; blkY++)
	{
		for (u32 blkX = 0; blkX < 128; blkX++)
		{
			if (GSTextureBlockIndex(blkX, blkY) != (blkY << 7) + blkX)
				mismatches++;
		}
	}
	EXPECT_EQ(mismatches, 0u);
}

TEST(GSTextureBlockIndex, WrapsLikeSourceUpdate)
{
	EXPECT_EQ(GSTextureBlockIndex(127, 255), ((255u << 7) + 127u) % GS_MAX_BLOCKS);
	EXPECT_EQ(GSTextureBlockIndex(0, 128), 0u);
}
