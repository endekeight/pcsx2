// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/GSRegs.h"
#include "GS/GSVector.h"

#include <algorithm>
#include <cstdint>

namespace GSChunkedKick
{

static constexpr u32 kMaxChunkVertices = 128;

struct DecidedChunk
{
	u64 accept[2] = {0, 0};
	GSVector4i draw_rect = GSVector4i::zero();
	u32 accepted_count = 0;

	__forceinline bool IsAccepted(u32 i) const
	{
		return (accept[i >> 6] & (1ULL << (i & 63))) != 0;
	}
};

struct VertexInput
{
	u16 x = 0;
	u16 y = 0;
	u32 skip = 0;
};

inline DecidedChunk DecideTriangleStrip(
	const VertexInput* RESTRICT verts,
	u32 count,
	const GSVector4i* carried_xy,
	u32 carried_count,
	const GSVector4i& xyof,
	const GSVector4i& scissor_cull,
	bool scissor_invalid,
	bool nativeres,
	bool aa1_expand = false)
{
	DecidedChunk result;
	const u32 n = std::min<u32>(count, kMaxChunkVertices);
	if (n == 0)
		return result;

	GSVector4i window[3];
	u32 window_len = std::min<u32>(carried_count, 2);

	if (window_len == 1)
	{
		window[0] = carried_xy[0];
	}
	else if (window_len == 2)
	{
		window[0] = carried_xy[0];
		window[1] = carried_xy[1];
	}

	for (u32 i = 0; i < n; ++i)
	{
		const GSVector4i xy_raw(static_cast<int>(verts[i].x), static_cast<int>(verts[i].y),
		                        static_cast<int>(verts[i].x), static_cast<int>(verts[i].y));
		const GSVector4i xy_i = xy_raw.sub32(xyof);

		if (window_len == 0)
		{
			window[0] = xy_i;
			window_len = 1;
			continue;
		}
		else if (window_len == 1)
		{
			window[1] = xy_i;
			window_len = 2;
			continue;
		}
		else
		{
			window[2] = xy_i;
		}

		const GSVector4i& v0 = window[2];
		const GSVector4i& v1 = window[1];
		const GSVector4i& v2 = window[0];

		u32 skip = (verts[i].skip != 0) ? 1u : 0u;
		skip |= static_cast<u32>(scissor_invalid);

		if (skip == 0)
		{
			GSVector4i bbox = v0.runion(v1).runion(v2);

			if (nativeres)
			{
				const GSVector4i interior = (bbox + GSVector4i(0xF, 0xF, -1, -1)) & GSVector4i(~0xF);
				bbox = interior + GSVector4i(0, 0, 1, 1);
			}
			else
			{
				bbox -= ((bbox & GSVector4i(0xF)) == GSVector4i(0)) & GSVector4i(0, 0, 1, 1);
			}

			if (aa1_expand)
			{
				bbox += GSVector4i(-0x10, -0x10, 0x10, 0x10);
			}

			const GSVector4i bbox_ex = bbox + GSVector4i(0, 0, 1, 1);
			u32 test = static_cast<u32>(!bbox_ex.rintersects(scissor_cull));
			test |= static_cast<u32>(bbox.rempty());
			test |= static_cast<u32>(v0.eq(v1)) | static_cast<u32>(v1.eq(v2)) | static_cast<u32>(v0.eq(v2));

			skip |= test;

			if (skip == 0)
			{
				result.accept[i >> 6] |= (1ULL << (i & 63));
				const GSVector4i prim_rect = bbox.sra32<4>() + GSVector4i(0, 0, 1, 1);
				if (result.accepted_count == 0)
					result.draw_rect = prim_rect;
				else
					result.draw_rect = result.draw_rect.runion(prim_rect);
				result.accepted_count++;
			}
		}

		window[0] = window[1];
		window[1] = window[2];
	}

	return result;
}

inline DecidedChunk DecideTriangleStripPacked(
	const GIFPackedReg* RESTRICT r,
	u32 count,
	const GSVector4i* carried_xy,
	u32 carried_count,
	const GSVector4i& xyof,
	const GSVector4i& scissor_cull,
	bool scissor_invalid,
	bool nativeres,
	bool aa1_expand = false)
{
	VertexInput inputs[kMaxChunkVertices];
	const u32 n = std::min<u32>(count, kMaxChunkVertices);
	for (u32 i = 0; i < n; ++i)
	{
		inputs[i].x = r[3 * i + 2].XYZF2.X;
		inputs[i].y = r[3 * i + 2].XYZF2.Y;
		inputs[i].skip = r[3 * i + 2].XYZF2.Skip();
	}
	return DecideTriangleStrip(inputs, n, carried_xy, carried_count, xyof, scissor_cull, scissor_invalid, nativeres, aa1_expand);
}

} // namespace GSChunkedKick
