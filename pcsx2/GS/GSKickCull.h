// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/GSRegs.h"
#include "GS/GSVector.h"

// The cull tests of GSState::VertexKick, kept verbatim as the reference for GSKickCullMask.
template <int primclass>
__forceinline u32 GSKickCullReference(const GSVector4i& bbox, const GSVector4i& scissor, const GSVector4i& v0, const GSVector4i& v1, const GSVector4i& v2)
{
	const GSVector4i bbox_ex = bbox + GSVector4i(0, 0, 1, 1);
	u32 test = static_cast<u32>(!bbox_ex.rintersects(scissor));

	if constexpr (primclass == GS_TRIANGLE_CLASS || primclass == GS_SPRITE_CLASS)
	{
		test |= static_cast<u32>(bbox.rempty());
	}

	if constexpr (primclass == GS_TRIANGLE_CLASS)
	{
		test |= static_cast<u32>(v0.eq(v1)) | static_cast<u32>(v1.eq(v2)) | static_cast<u32>(v0.eq(v2));
	}

	return test;
}

// Same result as GSKickCullReference for every input,
// computed as one lane mask with one reduction (C111 lever (c)).
template <int primclass>
__forceinline u32 GSKickCullMask(const GSVector4i& bbox, const GSVector4i& scissor, const GSVector4i& v0, const GSVector4i& v1, const GSVector4i& v2)
{
	const GSVector4i bbox_ex = bbox + GSVector4i(0, 0, 1, 1);
	const GSVector4i inter = bbox_ex.rintersect(scissor);
	GSVector4i bad;
	if constexpr (primclass == GS_TRIANGLE_CLASS || primclass == GS_SPRITE_CLASS)
	{
		// Lanes 0-1: intersection x0 < x1, y0 < y1. Lanes 2-3: bbox x0 < x1, y0 < y1.
		bad = ~inter.upl64(bbox).lt32(inter.uph64(bbox));
	}
	else
	{
		bad = ~inter.upl64(inter).lt32(inter.uph64(inter));
	}
	if constexpr (primclass == GS_TRIANGLE_CLASS)
	{
		GSVector4i e01 = v0.eq32(v1);
		GSVector4i e12 = v1.eq32(v2);
		GSVector4i e02 = v0.eq32(v2);
		e01 = e01 & e01.zwxy();
		e12 = e12 & e12.zwxy();
		e02 = e02 & e02.zwxy();
		e01 = e01 & e01.yxwz();
		e12 = e12 & e12.yxwz();
		e02 = e02 & e02.yxwz();
		bad = bad | e01 | e12 | e02;
	}
	return static_cast<u32>(!bad.allfalse());
}
