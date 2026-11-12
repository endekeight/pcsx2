// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/GSRegs.h"
#include "GS/GSVector.h"
#include "GS/Renderers/Common/GSVertex.h"

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <cstring>

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

struct ConvertJob
{
	u16 from = 0;
	u16 to = 0;
	u32 slot = 0;
};

struct PlannedChunk
{
	ConvertJob jobs[kMaxChunkVertices];
	u32 job_count = 0;
	u32 pre_copy_src = 0;
	u32 pre_copy_dst = 0;
	u32 pre_copy_count = 0;
	u32 head = 0;
	u32 tail = 0;
	u32 next = 0;
	u32 itail = 0;
	u32 trailing_from = 0;
	u32 trailing_to = 0;
	u32 trailing_slot = 0;
};

inline PlannedChunk PlanTriangleStrip(
	const u64 accept[2],
	u32 count,
	u32 head0,
	u32 tail0,
	u32 next0,
	u32 itail0,
	u16* RESTRICT idx_buff,
	bool convert_trailing = true)
{
	PlannedChunk plan;
	const u32 n = std::min<u32>(count, kMaxChunkVertices);

	u32 head = head0;
	u32 tail = tail0;
	u32 next = next0;
	u32 itail = itail0;

	if (n == 0)
	{
		plan.head = head;
		plan.tail = tail;
		plan.next = next;
		plan.itail = itail;
		return plan;
	}

	constexpr u32 kUnneededSlot = ~0u;
	u32 final_slot[kMaxChunkVertices];
	bool needed[kMaxChunkVertices] = {};
	for (u32 i = 0; i < n; ++i)
		final_slot[i] = kUnneededSlot;

	u32 last_accepted_plus_one = 0;

	for (u32 i = 0; i < n; ++i)
	{
		tail = tail + 1;

		if (tail - head < 3)
			continue;

		const bool accepted = (accept[i >> 6] & (1ULL << (i & 63))) != 0;
		if (!accepted)
		{
			head = head + 1;
			continue;
		}

		if (next < head)
		{
			if (i == 0)
			{
				plan.pre_copy_src = head;
				plan.pre_copy_dst = next;
				plan.pre_copy_count = 2;
			}
			else if (i == 1)
			{
				plan.pre_copy_src = head;
				plan.pre_copy_dst = next;
				plan.pre_copy_count = 1;
			}
			head = next;
			tail = next + 3;
		}

		for (u32 w = 0; w < 3; ++w)
		{
			if (i + w >= 2)
			{
				const u32 v_idx = i + w - 2;
				final_slot[v_idx] = head + w;
				needed[v_idx] = true;
			}
		}

		idx_buff[itail + 0] = static_cast<u16>(head + 0);
		idx_buff[itail + 1] = static_cast<u16>(head + 1);
		idx_buff[itail + 2] = static_cast<u16>(head + 2);
		itail += 3;

		next = head + 3;
		head = head + 1;
		last_accepted_plus_one = i + 1;
	}

	if (last_accepted_plus_one > 0)
	{
		plan.trailing_from = last_accepted_plus_one;
		plan.trailing_to = n;
		plan.trailing_slot = next;
	}
	else
	{
		plan.trailing_from = 0;
		plan.trailing_to = n;
		plan.trailing_slot = tail0;
	}

	if (convert_trailing)
	{
		for (u32 i = plan.trailing_from; i < plan.trailing_to; ++i)
		{
			needed[i] = true;
			final_slot[i] = plan.trailing_slot + (i - plan.trailing_from);
		}
	}
	else
	{
		const u32 carry_out = std::min<u32>(tail - head, n);
		for (u32 c = 0; c < carry_out; ++c)
		{
			const u32 idx = n - carry_out + c;
			needed[idx] = true;
			final_slot[idx] = tail - carry_out + c;
		}
	}

	for (u32 i = 0; i < n; ++i)
	{
		if (!needed[i])
			continue;

		const u32 slot = final_slot[i];
		if (plan.job_count > 0)
		{
			ConvertJob& prev = plan.jobs[plan.job_count - 1];
			if (prev.to == i && prev.slot + (prev.to - prev.from) == slot)
			{
				prev.to = static_cast<u16>(i + 1);
				continue;
			}
		}

		ConvertJob& job = plan.jobs[plan.job_count++];
		job.from = static_cast<u16>(i);
		job.to = static_cast<u16>(i + 1);
		job.slot = slot;
	}

	plan.head = head;
	plan.tail = tail;
	plan.next = next;
	plan.itail = itail;
	return plan;
}

enum class DepthLimitMode : u8
{
	Disabled = 0,
	PrioritizeUpper = 1,
	PrioritizeLower = 2,
};

inline void ConvertPackedSTQRGBAXYZF2Vertex(
	const GIFPackedReg* RESTRICT v_r,
	u32 uv,
	DepthLimitMode depth_limit,
	GSVertex& dst)
{
	const GSVector4i st = GSVector4i::loadl(&v_r[0].U64[0]);
	GSVector4i q = GSVector4i::loadl(&v_r[0].U64[1]);
	const GSVector4i rgba = (GSVector4i::load<false>(&v_r[1]) & GSVector4i::x000000ff()).ps32().pu16();

	q = q.blend8(GSVector4i::cast(GSVector4(FLT_MIN)), q == GSVector4i::zero());

	const GSVector4i v0 = st.upl64(rgba.upl32(q));

	GSVector4i xy = GSVector4i::loadl(&v_r[2].U64[0]);
	GSVector4i zf = GSVector4i::loadl(&v_r[2].U64[1]);
	xy = xy.upl16(xy.srl<4>()).upl32(GSVector4i::load(static_cast<int>(uv)));
	zf = zf.srl32<4>() & GSVector4i::x00ffffff().upl32(GSVector4i::x000000ff());

	GSVector4i v1 = xy.upl32(zf);

	dst.m[0] = v0;
	dst.m[1] = v1;

	if (depth_limit == DepthLimitMode::PrioritizeUpper)
		dst.XYZ.Z = ((dst.XYZ.Z >> 8) & ~0xFFu) | (dst.XYZ.Z & 0xFFu);
	else if (depth_limit == DepthLimitMode::PrioritizeLower)
		dst.XYZ.Z &= 0x00FFFFFFu;
}

inline void ConvertTriangleStripChunk(
	const GIFPackedReg* RESTRICT r,
	u32 count,
	const PlannedChunk& plan,
	u32 uv,
	const GSVector4i& xyof,
	DepthLimitMode depth_limit,
	GSVertex* RESTRICT vtx_buff,
	GSVector4i xy_ring[4],
	u32& xy_tail,
	GSVertex& last_v,
	float& last_q)
{
	const u32 n = std::min<u32>(count, kMaxChunkVertices);
	if (n == 0)
		return;

	if (plan.pre_copy_count > 0 && plan.pre_copy_dst != plan.pre_copy_src)
	{
		for (u32 c = 0; c < plan.pre_copy_count; ++c)
			vtx_buff[plan.pre_copy_dst + c] = vtx_buff[plan.pre_copy_src + c];
	}

	const u32 last_idx = n - 1;
	bool last_converted = false;
	u32 last_slot = 0;

	for (u32 j = 0; j < plan.job_count; ++j)
	{
		const ConvertJob& job = plan.jobs[j];
		u32 slot = job.slot;
		for (u32 i = job.from; i < job.to; ++i, ++slot)
		{
			ConvertPackedSTQRGBAXYZF2Vertex(r + 3 * i, uv, depth_limit, vtx_buff[slot]);
			if (i == last_idx)
			{
				last_converted = true;
				last_slot = slot;
			}
		}
	}

	if (last_converted)
	{
		last_v = vtx_buff[last_slot];
	}
	else
	{
		ConvertPackedSTQRGBAXYZF2Vertex(r + 3 * last_idx, uv, depth_limit, last_v);
	}

	last_q = r[3 * last_idx].STQ.Q;

	const u32 ring_writes = std::min<u32>(n, 4);
	const u32 start_i = n - ring_writes;
	for (u32 k = 0; k < ring_writes; ++k)
	{
		const u32 i = start_i + k;
		const int rx = static_cast<int>(r[3 * i + 2].XYZF2.X);
		const int ry = static_cast<int>(r[3 * i + 2].XYZF2.Y);
		const GSVector4i xy = GSVector4i(rx, ry, rx, ry).sub32(xyof);
		xy_ring[(xy_tail + start_i + k) & 3] = xy;
	}
	xy_tail += n;
}

inline void ConvertVertexRange(
	const GIFPackedReg* RESTRICT r,
	u32 from_idx,
	u32 to_idx,
	u32 dst_slot,
	u32 uv,
	DepthLimitMode depth_limit,
	GSVertex* RESTRICT vtx_buff)
{
	u32 slot = dst_slot;
	for (u32 i = from_idx; i < to_idx; ++i, ++slot)
		ConvertPackedSTQRGBAXYZF2Vertex(r + 3 * i, uv, depth_limit, vtx_buff[slot]);
}

} // namespace GSChunkedKick
