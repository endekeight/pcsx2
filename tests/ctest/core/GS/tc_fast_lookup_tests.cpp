// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSRegs.h"

#include <gtest/gtest.h>

namespace
{
	// Keep in sync with GSTextureCache::SourceMap::AnyPageUsed.
	bool AnyPageUsed(const u64* bits, u32 start, u32 count)
	{
		if (count == 0)
			return false;
		if (count >= GS_MAX_PAGES)
		{
			for (u32 w = 0; w < GS_MAX_PAGES / 64; w++)
			{
				if (bits[w])
					return true;
			}
			return false;
		}

		auto range_used = [bits](u32 first, u32 last) -> bool {
			if (first >= last)
				return false;
			const u32 first_w = first / 64;
			const u32 last_w = (last - 1) / 64;
			const u32 first_b = first % 64;
			if (first_w == last_w)
			{
				const u32 width = last - first;
				const u64 mask = (width == 64) ? ~u64{0} : (((u64{1} << width) - 1) << first_b);
				return (bits[first_w] & mask) != 0;
			}
			if ((bits[first_w] >> first_b) != 0)
				return true;
			for (u32 w = first_w + 1; w < last_w; w++)
			{
				if (bits[w])
					return true;
			}
			const u32 last_b = last % 64;
			const u64 last_mask = (last_b == 0) ? ~u64{0} : ((u64{1} << last_b) - 1);
			return (bits[last_w] & last_mask) != 0;
		};

		start %= GS_MAX_PAGES;
		const u32 end = start + count;
		if (end <= GS_MAX_PAGES)
			return range_used(start, end);
		return range_used(start, GS_MAX_PAGES) || range_used(0, end - GS_MAX_PAGES);
	}

	void SetPage(u64* bits, u32 page)
	{
		page %= GS_MAX_PAGES;
		bits[page / 64] |= u64{1} << (page % 64);
	}
} // namespace

TEST(GSTextureCacheAnyPageUsed, EmptyIsUnused)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	EXPECT_FALSE(AnyPageUsed(bits, 0, 1));
	EXPECT_FALSE(AnyPageUsed(bits, 0, GS_MAX_PAGES));
	EXPECT_FALSE(AnyPageUsed(bits, 100, 8));
}

TEST(GSTextureCacheAnyPageUsed, CountZeroIgnoresBits)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 0);
	EXPECT_FALSE(AnyPageUsed(bits, 0, 0));
	EXPECT_FALSE(AnyPageUsed(bits, 3, 0));
}

TEST(GSTextureCacheAnyPageUsed, SingleBit)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 0);
	EXPECT_TRUE(AnyPageUsed(bits, 0, 1));
	EXPECT_FALSE(AnyPageUsed(bits, 1, 1));
	EXPECT_TRUE(AnyPageUsed(bits, 0, 2));
}

TEST(GSTextureCacheAnyPageUsed, WordBoundary)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 63);
	SetPage(bits, 64);
	EXPECT_TRUE(AnyPageUsed(bits, 63, 1));
	EXPECT_TRUE(AnyPageUsed(bits, 64, 1));
	EXPECT_TRUE(AnyPageUsed(bits, 63, 2));
	EXPECT_FALSE(AnyPageUsed(bits, 62, 1));
	EXPECT_FALSE(AnyPageUsed(bits, 65, 1));
	EXPECT_TRUE(AnyPageUsed(bits, 0, 64));
	EXPECT_TRUE(AnyPageUsed(bits, 64, 64));
}

TEST(GSTextureCacheAnyPageUsed, WrapPast511)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 510);
	SetPage(bits, 0);
	EXPECT_TRUE(AnyPageUsed(bits, 510, 4));
	EXPECT_TRUE(AnyPageUsed(bits, 511, 2));
	EXPECT_FALSE(AnyPageUsed(bits, 511, 1));
	EXPECT_TRUE(AnyPageUsed(bits, 512 + 510, 1));
}

TEST(GSTextureCacheAnyPageUsed, CountAtLeastMaxPages)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	EXPECT_FALSE(AnyPageUsed(bits, 7, GS_MAX_PAGES));
	EXPECT_FALSE(AnyPageUsed(bits, 0, GS_MAX_PAGES + 8));
	SetPage(bits, 400);
	EXPECT_TRUE(AnyPageUsed(bits, 0, GS_MAX_PAGES));
	EXPECT_TRUE(AnyPageUsed(bits, 12, GS_MAX_PAGES + 1));
}
