// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSVertexKickChunked.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <cstring>

namespace
{

struct ReferenceDecideResult
{
	u64 accept[2] = {0, 0};
	GSVector4i draw_rect = GSVector4i::zero();
	u32 accepted_count = 0;

	bool IsAccepted(u32 i) const
	{
		return (accept[i >> 6] & (1ULL << (i & 63))) != 0;
	}
};

static ReferenceDecideResult ReferenceDecideTriangleStrip(
	const GSChunkedKick::VertexInput* verts,
	u32 count,
	const GSVector4i* carried_xy,
	u32 carried_count,
	const GSVector4i& xyof,
	const GSVector4i& scissor_cull,
	bool scissor_invalid,
	bool nativeres,
	bool aa1_expand = false)
{
	ReferenceDecideResult ref;
	const u32 n = std::min<u32>(count, GSChunkedKick::kMaxChunkVertices);
	if (n == 0)
		return ref;

	GSVector4i history[GSChunkedKick::kMaxChunkVertices + 2];
	u32 hist_len = 0;

	if (carried_count == 1)
	{
		history[0] = carried_xy[0];
		hist_len = 1;
	}
	else if (carried_count >= 2)
	{
		history[0] = carried_xy[0];
		history[1] = carried_xy[1];
		hist_len = 2;
	}

	for (u32 i = 0; i < n; ++i)
	{
		const int rx = static_cast<int>(verts[i].x);
		const int ry = static_cast<int>(verts[i].y);
		const GSVector4i xy_cur = GSVector4i(rx, ry, rx, ry).sub32(xyof);

		history[hist_len] = xy_cur;
		hist_len++;

		if (hist_len < 3)
			continue;

		const GSVector4i v0 = history[hist_len - 1];
		const GSVector4i v1 = history[hist_len - 2];
		const GSVector4i v2 = history[hist_len - 3];

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
			const bool outside = !bbox_ex.rintersects(scissor_cull);
			const bool empty = bbox.rempty();
			const bool degen = (v0.eq(v1) || v1.eq(v2) || v0.eq(v2));

			if (!outside && !empty && !degen)
			{
				ref.accept[i >> 6] |= (1ULL << (i & 63));
				const GSVector4i prim_rect = bbox.sra32<4>() + GSVector4i(0, 0, 1, 1);
				if (ref.accepted_count == 0)
					ref.draw_rect = prim_rect;
				else
					ref.draw_rect = ref.draw_rect.runion(prim_rect);
				ref.accepted_count++;
			}
		}
	}

	return ref;
}

static void VerifyDecideMatchesReference(
	const GSChunkedKick::VertexInput* verts,
	u32 count,
	const GSVector4i* carried_xy,
	u32 carried_count,
	const GSVector4i& xyof,
	const GSVector4i& scissor_cull,
	bool scissor_invalid,
	bool nativeres,
	bool aa1_expand = false)
{
	const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
		verts, count, carried_xy, carried_count, xyof, scissor_cull, scissor_invalid, nativeres, aa1_expand);

	const ReferenceDecideResult expected = ReferenceDecideTriangleStrip(
		verts, count, carried_xy, carried_count, xyof, scissor_cull, scissor_invalid, nativeres, aa1_expand);

	EXPECT_EQ(actual.accept[0], expected.accept[0]);
	EXPECT_EQ(actual.accept[1], expected.accept[1]);
	EXPECT_EQ(actual.accepted_count, expected.accepted_count);
	if (expected.accepted_count > 0)
	{
		EXPECT_TRUE(actual.draw_rect.eq(expected.draw_rect));
	}
}

} // namespace

TEST(GSKickChunkedTests, AllAccepted)
{
	const GSVector4i scissor(0, 0, 4096, 4096);
	const GSVector4i xyof = GSVector4i::zero();
	const GSChunkedKick::VertexInput verts[] = {
		{100 << 4, 100 << 4, 0},
		{120 << 4, 180 << 4, 0},
		{150 << 4, 100 << 4, 0},
		{180 << 4, 190 << 4, 0},
		{210 << 4, 110 << 4, 0},
		{240 << 4, 200 << 4, 0},
	};
	const u32 count = sizeof(verts) / sizeof(verts[0]);
	VerifyDecideMatchesReference(verts, count, nullptr, 0, xyof, scissor, false, true);

	const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
		verts, count, nullptr, 0, xyof, scissor, false, true);
	EXPECT_EQ(actual.accept[0], (1ULL << 2) | (1ULL << 3) | (1ULL << 4) | (1ULL << 5));
	EXPECT_EQ(actual.accept[1], 0ULL);
	EXPECT_EQ(actual.accepted_count, 4u);
}

TEST(GSKickChunkedTests, AllOutsideScissor)
{
	const GSVector4i scissor(100, 100, 200, 200);
	const GSVector4i xyof = GSVector4i::zero();
	const GSChunkedKick::VertexInput verts[] = {
		{500 << 4, 500 << 4, 0},
		{520 << 4, 580 << 4, 0},
		{550 << 4, 500 << 4, 0},
		{580 << 4, 590 << 4, 0},
	};
	const u32 count = sizeof(verts) / sizeof(verts[0]);
	VerifyDecideMatchesReference(verts, count, nullptr, 0, xyof, scissor, false, true);

	const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
		verts, count, nullptr, 0, xyof, scissor, false, true);
	EXPECT_EQ(actual.accept[0], 0ULL);
	EXPECT_EQ(actual.accept[1], 0ULL);
	EXPECT_EQ(actual.accepted_count, 0u);
}

TEST(GSKickChunkedTests, ScissorInvalid)
{
	const GSVector4i scissor(0, 0, 4096, 4096);
	const GSVector4i xyof = GSVector4i::zero();
	const GSChunkedKick::VertexInput verts[] = {
		{100 << 4, 100 << 4, 0},
		{120 << 4, 180 << 4, 0},
		{150 << 4, 100 << 4, 0},
		{180 << 4, 190 << 4, 0},
	};
	const u32 count = sizeof(verts) / sizeof(verts[0]);
	VerifyDecideMatchesReference(verts, count, nullptr, 0, xyof, scissor, true, true);

	const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
		verts, count, nullptr, 0, xyof, scissor, true, true);
	EXPECT_EQ(actual.accept[0], 0ULL);
	EXPECT_EQ(actual.accept[1], 0ULL);
	EXPECT_EQ(actual.accepted_count, 0u);
}

TEST(GSKickChunkedTests, DegenerateTrianglesEachPairing)
{
	const GSVector4i scissor(0, 0, 4096, 4096);
	const GSVector4i xyof = GSVector4i::zero();

	// Pairing 1: v0 == v1 (vertex 2 == vertex 1)
	{
		const GSChunkedKick::VertexInput verts[] = {
			{100 << 4, 100 << 4, 0},
			{120 << 4, 150 << 4, 0},
			{120 << 4, 150 << 4, 0},
			{180 << 4, 110 << 4, 0},
			{210 << 4, 150 << 4, 0},
		};
		VerifyDecideMatchesReference(verts, 5, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 5, nullptr, 0, xyof, scissor, false, true);
		EXPECT_FALSE(actual.IsAccepted(2));
		EXPECT_FALSE(actual.IsAccepted(3));
		EXPECT_TRUE(actual.IsAccepted(4));
	}

	// Pairing 2: v1 == v2 (vertex 1 == vertex 0)
	{
		const GSChunkedKick::VertexInput verts[] = {
			{100 << 4, 100 << 4, 0},
			{100 << 4, 100 << 4, 0},
			{150 << 4, 150 << 4, 0},
			{180 << 4, 110 << 4, 0},
		};
		VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 4, nullptr, 0, xyof, scissor, false, true);
		EXPECT_FALSE(actual.IsAccepted(2));
		EXPECT_TRUE(actual.IsAccepted(3));
	}

	// Pairing 3: v0 == v2 (vertex 2 == vertex 0)
	{
		const GSChunkedKick::VertexInput verts[] = {
			{100 << 4, 100 << 4, 0},
			{120 << 4, 150 << 4, 0},
			{100 << 4, 100 << 4, 0},
			{180 << 4, 110 << 4, 0},
		};
		VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 4, nullptr, 0, xyof, scissor, false, true);
		EXPECT_FALSE(actual.IsAccepted(2));
		EXPECT_TRUE(actual.IsAccepted(3));
	}
}

TEST(GSKickChunkedTests, EmptyBox)
{
	const GSVector4i scissor(0, 0, 4096, 4096);
	const GSVector4i xyof = GSVector4i::zero();

	// Collinear vertices on a horizontal line (all same Y, different X)
	const GSChunkedKick::VertexInput verts[] = {
		{100 << 4, 200 << 4, 0},
		{150 << 4, 200 << 4, 0},
		{200 << 4, 200 << 4, 0},
		{220 << 4, 300 << 4, 0},
	};
	VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, true);
	const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
		verts, 4, nullptr, 0, xyof, scissor, false, true);
	EXPECT_FALSE(actual.IsAccepted(2));
	EXPECT_TRUE(actual.IsAccepted(3));
}

TEST(GSKickChunkedTests, SkipBitOnCompletingAndNonCompleting)
{
	const GSVector4i scissor(0, 0, 4096, 4096);
	const GSVector4i xyof = GSVector4i::zero();

	// Case A: Skip bit set on non-completing vertex 0
	{
		const GSChunkedKick::VertexInput verts[] = {
			{100 << 4, 100 << 4, 1},
			{120 << 4, 180 << 4, 0},
			{150 << 4, 100 << 4, 0},
		};
		VerifyDecideMatchesReference(verts, 3, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 3, nullptr, 0, xyof, scissor, false, true);
		EXPECT_TRUE(actual.IsAccepted(2));
	}

	// Case B: Skip bit set on non-completing vertex 1
	{
		const GSChunkedKick::VertexInput verts[] = {
			{100 << 4, 100 << 4, 0},
			{120 << 4, 180 << 4, 1},
			{150 << 4, 100 << 4, 0},
		};
		VerifyDecideMatchesReference(verts, 3, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 3, nullptr, 0, xyof, scissor, false, true);
		EXPECT_TRUE(actual.IsAccepted(2));
	}

	// Case C: Skip bit set on completing vertex 2
	{
		const GSChunkedKick::VertexInput verts[] = {
			{100 << 4, 100 << 4, 0},
			{120 << 4, 180 << 4, 0},
			{150 << 4, 100 << 4, 1},
			{180 << 4, 190 << 4, 0},
		};
		VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 4, nullptr, 0, xyof, scissor, false, true);
		EXPECT_FALSE(actual.IsAccepted(2));
		EXPECT_TRUE(actual.IsAccepted(3));
	}
}

TEST(GSKickChunkedTests, CarriedInVertices)
{
	const GSVector4i scissor(0, 0, 4096, 4096);
	const GSVector4i xyof = GSVector4i::zero();
	const GSChunkedKick::VertexInput verts[] = {
		{100 << 4, 100 << 4, 0},
		{120 << 4, 180 << 4, 0},
		{150 << 4, 100 << 4, 0},
	};

	// 0 carried-in vertices
	{
		VerifyDecideMatchesReference(verts, 3, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 3, nullptr, 0, xyof, scissor, false, true);
		EXPECT_FALSE(actual.IsAccepted(0));
		EXPECT_FALSE(actual.IsAccepted(1));
		EXPECT_TRUE(actual.IsAccepted(2));
		EXPECT_EQ(actual.accepted_count, 1u);
	}

	// 1 carried-in vertex
	{
		const GSVector4i carried[1] = {
			GSVector4i(80 << 4, 120 << 4, 80 << 4, 120 << 4),
		};
		VerifyDecideMatchesReference(verts, 3, carried, 1, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 3, carried, 1, xyof, scissor, false, true);
		EXPECT_FALSE(actual.IsAccepted(0));
		EXPECT_TRUE(actual.IsAccepted(1));
		EXPECT_TRUE(actual.IsAccepted(2));
		EXPECT_EQ(actual.accepted_count, 2u);
	}

	// 2 carried-in vertices
	{
		const GSVector4i carried[2] = {
			GSVector4i(60 << 4, 100 << 4, 60 << 4, 100 << 4),
			GSVector4i(80 << 4, 120 << 4, 80 << 4, 120 << 4),
		};
		VerifyDecideMatchesReference(verts, 3, carried, 2, xyof, scissor, false, true);
		const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
			verts, 3, carried, 2, xyof, scissor, false, true);
		EXPECT_TRUE(actual.IsAccepted(0));
		EXPECT_TRUE(actual.IsAccepted(1));
		EXPECT_TRUE(actual.IsAccepted(2));
		EXPECT_EQ(actual.accepted_count, 3u);
	}
}

TEST(GSKickChunkedTests, Full128VertexChunk)
{
	const GSVector4i scissor(50 << 4, 50 << 4, 2000 << 4, 2000 << 4);
	const GSVector4i xyof = GSVector4i::zero();
	GSChunkedKick::VertexInput verts[128];

	for (u32 i = 0; i < 128; ++i)
	{
		verts[i].x = static_cast<u16>((100 + (i * 10)) << 4);
		verts[i].y = static_cast<u16>((100 + ((i & 1) ? 80 : 0)) << 4);
		verts[i].skip = 0;

		// Vertex 30: degenerate with vertex 29
		if (i == 30)
		{
			verts[i].x = verts[i - 1].x;
			verts[i].y = verts[i - 1].y;
		}
		// Vertex 70: skip bit set
		if (i == 70)
		{
			verts[i].skip = 1;
		}
		// Vertices 100-105: outside scissor
		if (i >= 100 && i <= 105)
		{
			verts[i].x = static_cast<u16>(3000 << 4);
			verts[i].y = static_cast<u16>(3000 << 4);
		}
	}

	VerifyDecideMatchesReference(verts, 128, nullptr, 0, xyof, scissor, false, true);

	const GSChunkedKick::DecidedChunk actual = GSChunkedKick::DecideTriangleStrip(
		verts, 128, nullptr, 0, xyof, scissor, false, true);
	EXPECT_NE(actual.accept[0], 0ULL);
	EXPECT_NE(actual.accept[1], 0ULL);
	EXPECT_FALSE(actual.IsAccepted(30));
	EXPECT_FALSE(actual.IsAccepted(70));
}

TEST(GSKickChunkedTests, NonZeroAndSubpixelXYOffset)
{
	const GSVector4i scissor(100, 100, 1000, 1000);

	// Integer offset
	{
		const GSVector4i xyof(2048 << 4, 2048 << 4, 2048 << 4, 2048 << 4);
		const GSChunkedKick::VertexInput verts[] = {
			{static_cast<u16>((2048 + 150) << 4), static_cast<u16>((2048 + 150) << 4), 0},
			{static_cast<u16>((2048 + 170) << 4), static_cast<u16>((2048 + 250) << 4), 0},
			{static_cast<u16>((2048 + 200) << 4), static_cast<u16>((2048 + 160) << 4), 0},
			{static_cast<u16>((2048 + 230) << 4), static_cast<u16>((2048 + 260) << 4), 0},
		};
		VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, true);
	}

	// Sub-pixel offset (fractional offset in lower 4 bits)
	{
		const GSVector4i xyof((2048 << 4) + 7, (2048 << 4) + 11, (2048 << 4) + 7, (2048 << 4) + 11);
		const GSChunkedKick::VertexInput verts[] = {
			{static_cast<u16>(((2048 + 150) << 4) + 3), static_cast<u16>(((2048 + 150) << 4) + 5), 0},
			{static_cast<u16>(((2048 + 170) << 4) + 8), static_cast<u16>(((2048 + 250) << 4) + 2), 0},
			{static_cast<u16>(((2048 + 200) << 4) + 1), static_cast<u16>(((2048 + 160) << 4) + 9), 0},
			{static_cast<u16>(((2048 + 230) << 4) + 6), static_cast<u16>(((2048 + 260) << 4) + 4), 0},
		};
		VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, true);
	}
}

TEST(GSKickChunkedTests, NativeAndNonNativeResolution)
{
	const GSVector4i scissor(50, 50, 500, 500);
	const GSVector4i xyof = GSVector4i::zero();

	const GSChunkedKick::VertexInput verts[] = {
		{100 << 4, 100 << 4, 0},
		{static_cast<u16>((100 << 4) + 8), static_cast<u16>((150 << 4) + 8), 0},
		{static_cast<u16>((150 << 4) + 8), static_cast<u16>((100 << 4) + 8), 0},
		{200 << 4, 180 << 4, 0},
	};

	// Native resolution
	VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, true);

	// Non-native (upscaled) resolution
	VerifyDecideMatchesReference(verts, 4, nullptr, 0, xyof, scissor, false, false);
}

TEST(GSKickChunkedTests, DecideTriangleStripPacked)
{
	const GSVector4i scissor(0, 0, 4096, 4096);
	const GSVector4i xyof = GSVector4i::zero();

	GIFPackedReg r[3 * 4];
	std::memset(r, 0, sizeof(r));

	// Vertex 0
	r[2].XYZF2.X = 100 << 4;
	r[2].XYZF2.Y = 100 << 4;
	r[2].U32[3] = 0;

	// Vertex 1
	r[5].XYZF2.X = 120 << 4;
	r[5].XYZF2.Y = 180 << 4;
	r[5].U32[3] = 0;

	// Vertex 2
	r[8].XYZF2.X = 150 << 4;
	r[8].XYZF2.Y = 100 << 4;
	r[8].U32[3] = 0;

	// Vertex 3
	r[11].XYZF2.X = 180 << 4;
	r[11].XYZF2.Y = 190 << 4;
	r[11].U32[3] = 0x8000; // skip = 1 (completing vertex rejected)

	const GSChunkedKick::DecidedChunk packed = GSChunkedKick::DecideTriangleStripPacked(
		r, 4, nullptr, 0, xyof, scissor, false, true);

	EXPECT_FALSE(packed.IsAccepted(0));
	EXPECT_FALSE(packed.IsAccepted(1));
	EXPECT_TRUE(packed.IsAccepted(2));
	EXPECT_FALSE(packed.IsAccepted(3));
	EXPECT_EQ(packed.accepted_count, 1u);
}

TEST(GSKickChunkedTests, AA1ExpansionChangesDecision)
{
	const GSVector4i scissor(1000, 1000, 2000, 2000);
	const GSVector4i xyof = GSVector4i::zero();
	const GSChunkedKick::VertexInput verts[] = {
		{950, 1150, 0},
		{980, 1200, 0},
		{1005, 1100, 0},
	};
	const u32 count = sizeof(verts) / sizeof(verts[0]);

	// With aa1_expand = false: primitive bounding box is outside scissor, decision is rejected
	VerifyDecideMatchesReference(verts, count, nullptr, 0, xyof, scissor, false, true, false);
	const GSChunkedKick::DecidedChunk actual_off = GSChunkedKick::DecideTriangleStrip(
		verts, count, nullptr, 0, xyof, scissor, false, true, false);
	EXPECT_EQ(actual_off.accept[0], 0ULL);
	EXPECT_EQ(actual_off.accepted_count, 0u);
	EXPECT_FALSE(actual_off.IsAccepted(2));

	// With aa1_expand = true: 1-pixel expansion brings primitive inside scissor, decision is accepted
	VerifyDecideMatchesReference(verts, count, nullptr, 0, xyof, scissor, false, true, true);
	const GSChunkedKick::DecidedChunk actual_on = GSChunkedKick::DecideTriangleStrip(
		verts, count, nullptr, 0, xyof, scissor, false, true, true);
	EXPECT_EQ(actual_on.accept[0], 1ULL << 2);
	EXPECT_EQ(actual_on.accepted_count, 1u);
	EXPECT_TRUE(actual_on.IsAccepted(2));
}
