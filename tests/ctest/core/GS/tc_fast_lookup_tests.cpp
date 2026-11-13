// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSRegs.h"
#include "GS/GSTextureCacheFastLookup.h"

#include <gtest/gtest.h>

namespace
{
	void SetPage(u64* bits, u32 page)
	{
		page %= GS_MAX_PAGES;
		bits[page / 64] |= u64{1} << (page % 64);
	}

	bool PageLiesInSpan(u32 page, u32 first, u32 count)
	{
		u64 bits[GS_MAX_PAGES / 64] = {};
		SetPage(bits, page);
		return GSTextureCacheAnyPageUsed(bits, first, count);
	}
} // namespace

TEST(GSTextureCacheAnyPageUsed, EmptyIsUnused)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 0, 1));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 0, GS_MAX_PAGES));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 100, 8));
}

TEST(GSTextureCacheAnyPageUsed, CountZeroIgnoresBits)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 0);
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 0, 0));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 3, 0));
}

TEST(GSTextureCacheAnyPageUsed, SingleBit)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 0);
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 0, 1));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 1, 1));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 0, 2));
}

TEST(GSTextureCacheAnyPageUsed, WordBoundary)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 63);
	SetPage(bits, 64);
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 63, 1));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 64, 1));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 63, 2));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 62, 1));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 65, 1));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 0, 64));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 64, 64));
}

TEST(GSTextureCacheAnyPageUsed, WrapPast511)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	SetPage(bits, 510);
	SetPage(bits, 0);
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 510, 4));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 511, 2));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 511, 1));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 512 + 510, 1));
}

TEST(GSTextureCacheAnyPageUsed, CountAtLeastMaxPages)
{
	u64 bits[GS_MAX_PAGES / 64] = {};
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 7, GS_MAX_PAGES));
	EXPECT_FALSE(GSTextureCacheAnyPageUsed(bits, 0, GS_MAX_PAGES + 8));
	SetPage(bits, 400);
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 0, GS_MAX_PAGES));
	EXPECT_TRUE(GSTextureCacheAnyPageUsed(bits, 12, GS_MAX_PAGES + 1));
}

TEST(GSTextureCacheFastLookupLoopPagesSpan, UnadjustedLoopPagesLieInSpan)
{
	struct Case
	{
		u32 bp;
		u32 bw;
		int page_w;
		int page_h;
		int left;
		int top;
		int right;
		int bottom;
	};

	const Case cases[] = {
		{0, 1, 64, 32, 0, 0, 64, 32},
		{0, 8, 64, 32, 0, 0, 512, 64},
		{0, 10, 64, 32, 16, 8, 200, 100},
		{1, 8, 64, 32, 0, 0, 128, 32},
		{0, 1, 128, 64, 0, 0, 128, 64},
		{0, 8, 128, 64, 0, 0, 256, 128},
		{0, 10, 128, 64, 0, 0, 320, 80},
		{1, 10, 128, 64, 0, 0, 256, 64},
		{0, 10, 64, 32, 0, 0, 640, 1920},
		{15872, 10, 64, 32, 0, 0, 640, 64},
	};

	for (const Case& c : cases)
	{
		const GSVector4i rect = GSVector4i::cxpr(c.left, c.top, c.right, c.bottom);
		u32 first = 0;
		u32 count = 0;
		ASSERT_TRUE(FastLookupLoopPagesSpan(c.bp, c.bw, c.page_w, c.page_h, rect, first, count))
			<< "bp=" << c.bp << " bw=" << c.bw << " pgs=" << c.page_w << "x" << c.page_h
			<< " rect=" << c.left << "," << c.top << "," << c.right << "," << c.bottom;

		const int topPg = c.top / c.page_h;
		const int botPg = (c.bottom + c.page_h - 1) / c.page_h;
		const int yCnt = botPg - topPg;
		const int bwPg = static_cast<int>((c.bw * 64u) / static_cast<u32>(c.page_w));
		const bool aligned = (c.bp & 0x1f) == 0;
		const int xStart = c.left / c.page_w;
		const int xEnd = ((c.right + c.page_w - 1) / c.page_w) + (aligned ? 0 : 1);
		int lineBP = static_cast<int>(c.bp >> 5) + topPg * bwPg;
		u32 visited = 0;
		for (int y = 0; y < yCnt; y++)
		{
			const u32 start = static_cast<u32>(lineBP + xStart);
			const u32 end = static_cast<u32>(lineBP + xEnd);
			for (u32 pos = start; pos < end; pos++)
			{
				const u32 page = pos % GS_MAX_PAGES;
				EXPECT_TRUE(PageLiesInSpan(page, first, count))
					<< "page=" << page << " first=" << first << " count=" << count
					<< " bp=" << c.bp << " bw=" << c.bw << " pgs=" << c.page_w << "x" << c.page_h
					<< " rect=" << c.left << "," << c.top << "," << c.right << "," << c.bottom;
				visited++;
			}
			lineBP += bwPg;
		}
		EXPECT_GT(visited, 0u) << "bp=" << c.bp << " bw=" << c.bw;
	}
}
