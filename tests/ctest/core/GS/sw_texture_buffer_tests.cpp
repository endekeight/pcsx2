// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSRegs.h"

#include <gtest/gtest.h>

TEST(GSSwTextureGeometryValid, AcceptsAllSizesUpToTen)
{
	for (u32 tw = 0; tw <= 10; tw++)
	{
		for (u32 th = 0; th <= 10; th++)
		{
			EXPECT_TRUE(GSSwTextureGeometryValid(tw, th, tw)) << "tw=" << tw << " th=" << th;
		}
	}
}

TEST(GSSwTextureGeometryValid, RejectsTwAboveTen)
{
	for (u32 tw = 11; tw <= 15; tw++)
	{
		EXPECT_FALSE(GSSwTextureGeometryValid(tw, 0, 10)) << "tw=" << tw;
	}
}

TEST(GSSwTextureGeometryValid, RejectsThAboveTen)
{
	for (u32 th = 11; th <= 15; th++)
	{
		EXPECT_FALSE(GSSwTextureGeometryValid(0, th, 10)) << "th=" << th;
	}
}

TEST(GSSwTextureGeometryValid, RejectsBothAboveTen)
{
	for (u32 tw = 11; tw <= 15; tw++)
	{
		for (u32 th = 11; th <= 15; th++)
		{
			EXPECT_FALSE(GSSwTextureGeometryValid(tw, th, 10)) << "tw=" << tw << " th=" << th;
		}
	}
}

TEST(GSSwTextureGeometryValid, RejectsPitchNarrowerThanWidth)
{
	// Base TW 11 gives sel.tw 0, pitch_log2 3, while level 1 still has TW 10.
	EXPECT_FALSE(GSSwTextureGeometryValid(10, 0, 3));
	EXPECT_TRUE(GSSwTextureGeometryValid(3, 3, 3));
	EXPECT_TRUE(GSSwTextureGeometryValid(5, 5, 6));
}

TEST(GSSwTextureBufferSize, LargeTextureDoesNotWrapToZero)
{
	// Checks the helper alone; Update now rejects TW 14 before sizing, so this guards
	// the helper against a return of the 32-bit product.
	// TW = TH = 14, non-palette: pitch_log2 14, shift 2, rows 1 << 14.
	// The old 32-bit multiply wrapped to 0 here.
	const u32 pitch_log2 = 14;
	const u32 shift = 2;
	const u32 rows = 1u << 14;
	const size_t bytes_written = (size_t{1} << pitch_log2 << shift) * rows;
	const size_t size = GSSwTextureBufferSize(pitch_log2, shift, rows);
	EXPECT_NE(size, 0u);
	EXPECT_GE(size, bytes_written);
}

TEST(GSSwTextureBufferSize, MatchesOld32BitExpressionForValidSizes)
{
	auto OldSize = [](u32 pitch_log2, u32 shift, u32 rows) -> size_t {
		return size_t(u32((1u << pitch_log2) << shift) * u32(rows) * 4u);
	};

	const u32 row_counts[] = {1, 8, 16, 32, 64, 128, 256, 512, 1024};

	for (u32 pitch_log2 = 0; pitch_log2 <= 10; pitch_log2++)
	{
		for (u32 shift : {0u, 2u})
		{
			for (u32 rows : row_counts)
			{
				EXPECT_EQ(GSSwTextureBufferSize(pitch_log2, shift, rows),
					OldSize(pitch_log2, shift, rows))
					<< "pitch_log2=" << pitch_log2 << " shift=" << shift << " rows=" << rows;
			}
		}
	}
}

TEST(GSSwTextureBufferSize, MipmapLevelSixRequiresReallocation)
{
	// Texture::Reset frees a kept buffer when the required size exceeds the allocated one; the
	// texture itself needs a GS renderer, so this pins the sizes that decision compares.
	const size_t small = GSSwTextureBufferSize(3, 2, 8);
	const size_t large = GSSwTextureBufferSize(6, 2, 8);
	EXPECT_EQ(small, 1024u);
	EXPECT_EQ(large, 8192u);
	EXPECT_GT(large, small);

	// 8 rows of pitch 256 write 2048 bytes; the old kept 1024-byte buffer overflowed.
	const size_t bytes_written = (size_t{1} << 6 << 2) * 8;
	EXPECT_EQ(bytes_written, 2048u);
	EXPECT_GT(bytes_written, small);
	EXPECT_LE(bytes_written, large);
}
