// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/GSRegs.h"

static inline bool GSTextureCacheAnyPageUsed(const u64* bits, u32 start, u32 count)
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

static inline bool FastLookupLoopPagesSpan(u32 bp, u32 bw, int page_w, int page_h, const GSVector4i& rect, u32& first, u32& count)
{
	if (rect.left < 0 || rect.top < 0 || rect.right < rect.left || rect.bottom < rect.top)
		return false;

	if (page_w <= 0 || page_h <= 0 || (page_w & (page_w - 1)) != 0 || (page_h & (page_h - 1)) != 0)
		return false;

	const int topPg = rect.top / page_h;
	const int botPg = (rect.bottom + page_h - 1) / page_h;
	const int yCnt = botPg - topPg;
	const int bwPg = static_cast<int>((bw * 64u) / static_cast<u32>(page_w));
	if (yCnt <= 0 || bwPg < 0)
		return false;

	const bool aligned = (bp & 0x1f) == 0;
	const int xStart = rect.left / page_w;
	const int xEnd = ((rect.right + page_w - 1) / page_w) + (aligned ? 0 : 1);
	if (xEnd <= xStart)
		return false;

	const int looper_bp = static_cast<int>(bp >> 5) + topPg * bwPg;
	const int first_pos = looper_bp + xStart;
	const int last_pos = looper_bp + (yCnt - 1) * bwPg + xEnd;
	if (first_pos < 0 || last_pos <= first_pos)
		return false;

	first = static_cast<u32>(first_pos);
	count = static_cast<u32>(last_pos - first_pos);
	return true;
}
