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

namespace
{

static constexpr u32 kTestMaxVertices = 512;
static constexpr u32 kTestMaxIndices = 1536;
static constexpr u8 kSentinelByte = 0xA5;

struct StripStateBuffer
{
	GSVertex vtx[kTestMaxVertices];
	u16 idx[kTestMaxIndices];
	u32 head = 0;
	u32 tail = 0;
	u32 next = 0;
	u32 itail = 0;
	u32 xy_tail = 0;
	GSVector4i xy[4] = {};
	GSVertex m_v = {};
	float m_q = 1.0f;

	void InitSentinel(u32 head0 = 0, u32 tail0 = 0, u32 next0 = 0, u32 itail0 = 0, u32 xy_tail0 = 0)
	{
		std::memset(vtx, kSentinelByte, sizeof(vtx));
		std::memset(idx, 0xFF, sizeof(idx));
		std::memset(&m_v, 0, sizeof(m_v));
		m_v.UV = 0x12345678u;
		m_q = 1.0f;
		head = head0;
		tail = tail0;
		next = next0;
		itail = itail0;
		xy_tail = xy_tail0;
		for (int i = 0; i < 4; ++i)
			xy[i] = GSVector4i(10 + i, 20 + i, 10 + i, 20 + i);
	}
};

static bool IsSentinelVertex(const GSVertex& v)
{
	const u8* bytes = reinterpret_cast<const u8*>(&v);
	for (size_t i = 0; i < sizeof(GSVertex); ++i)
	{
		if (bytes[i] != kSentinelByte)
			return false;
	}
	return true;
}

static void FillSyntheticPackedVertex(
	GIFPackedReg* r,
	u32 index,
	u16 x,
	u16 y,
	u32 skip = 0,
	float s = 0.0f,
	float t = 0.0f,
	float q = 1.0f,
	u32 z = 0x00112233u,
	u8 fog = 0x44u)
{
	std::memset(r, 0, 3 * sizeof(GIFPackedReg));
	r[0].STQ.S = (s != 0.0f) ? s : static_cast<float>(index + 1) * 0.25f;
	r[0].STQ.T = (t != 0.0f) ? t : static_cast<float>(index + 1) * 0.5f;
	r[0].STQ.Q = q;

	r[1].RGBA.R = static_cast<u8>((index * 17 + 11) & 0xFF);
	r[1].RGBA.G = static_cast<u8>((index * 31 + 23) & 0xFF);
	r[1].RGBA.B = static_cast<u8>((index * 47 + 37) & 0xFF);
	r[1].RGBA.A = static_cast<u8>((index * 61 + 53) & 0xFF);

	r[2].XYZF2.X = x;
	r[2].XYZF2.Y = y;
	r[2].U64[1] = (static_cast<u64>(z & 0x00FFFFFFu) << 4) |
	              (static_cast<u64>(fog) << 36) |
	              (skip ? (1ULL << 47) : 0ULL);
}

static void ReferencePerVertexStripChunk(
	const GIFPackedReg* r,
	u32 count,
	const u64 accept[2],
	const GSVector4i& xyof,
	GSChunkedKick::DepthLimitMode depth_limit,
	StripStateBuffer& state)
{
	for (u32 i = 0; i < count; ++i)
	{
		const GIFPackedReg* cur = r + 3 * i;
		const GSVector4i st = GSVector4i::loadl(&cur[0].U64[0]);
		GSVector4i q = GSVector4i::loadl(&cur[0].U64[1]);
		const GSVector4i rgba = (GSVector4i::load<false>(&cur[1]) & GSVector4i::x000000ff()).ps32().pu16();

		q = q.blend8(GSVector4i::cast(GSVector4(FLT_MIN)), q == GSVector4i::zero());
		state.m_v.m[0] = st.upl64(rgba.upl32(q));

		GSVector4i xy = GSVector4i::loadl(&cur[2].U64[0]);
		GSVector4i zf = GSVector4i::loadl(&cur[2].U64[1]);
		xy = xy.upl16(xy.srl<4>()).upl32(GSVector4i::load(static_cast<int>(state.m_v.UV)));
		zf = zf.srl32<4>() & GSVector4i::x00ffffff().upl32(GSVector4i::x000000ff());
		state.m_v.m[1] = xy.upl32(zf);

		if (depth_limit == GSChunkedKick::DepthLimitMode::PrioritizeUpper)
			state.m_v.XYZ.Z = ((state.m_v.XYZ.Z >> 8) & ~0xFFu) | (state.m_v.XYZ.Z & 0xFFu);
		else if (depth_limit == GSChunkedKick::DepthLimitMode::PrioritizeLower)
			state.m_v.XYZ.Z &= 0x00FFFFFFu;

		u32 head = state.head;
		u32 tail = state.tail;
		u32 next = state.next;
		u32 xy_tail = state.xy_tail;

		const GSVector4i new_v0(state.m_v.m[0]);
		const GSVector4i new_v1(state.m_v.m[1]);
		GSVector4i* tailptr = reinterpret_cast<GSVector4i*>(&state.vtx[tail]);
		tailptr[0] = new_v0;
		tailptr[1] = new_v1;

		const GSVector4i xy_sub = new_v1.xxxx().u16to32().sub32(xyof);
		state.xy[xy_tail & 3] = xy_sub;

		state.tail = ++tail;
		state.xy_tail = ++xy_tail;

		const u32 m = tail - head;
		if (m < 3)
			continue;

		const bool accepted = (accept[i >> 6] & (1ULL << (i & 63))) != 0;
		if (!accepted)
		{
			state.head = head + 1;
			continue;
		}

		if (next < head)
		{
			state.vtx[next + 0] = state.vtx[head + 0];
			state.vtx[next + 1] = state.vtx[head + 1];
			state.vtx[next + 2] = state.vtx[head + 2];
			head = next;
			state.tail = next + 3;
		}

		state.idx[state.itail + 0] = static_cast<u16>(head + 0);
		state.idx[state.itail + 1] = static_cast<u16>(head + 1);
		state.idx[state.itail + 2] = static_cast<u16>(head + 2);
		state.head = head + 1;
		state.next = head + 3;
		state.itail += 3;
	}

	if (count > 0)
		state.m_q = r[3 * (count - 1)].STQ.Q;
}

static void CompareChunkStateAgainstReference(
	const StripStateBuffer& actual,
	const StripStateBuffer& ref,
	u32 pre_head0 = 0,
	u32 pre_tail0 = 0)
{
	EXPECT_EQ(actual.head, ref.head);
	EXPECT_EQ(actual.tail, ref.tail);
	EXPECT_EQ(actual.next, ref.next);
	EXPECT_EQ(actual.itail, ref.itail);
	EXPECT_EQ(actual.xy_tail, ref.xy_tail);

	for (int i = 0; i < 4; ++i)
		EXPECT_TRUE(actual.xy[i].eq(ref.xy[i])) << "Mismatch in xy[" << i << "]";

	EXPECT_EQ(std::memcmp(&actual.m_v, &ref.m_v, sizeof(GSVertex)), 0);
	EXPECT_EQ(std::memcmp(&actual.m_q, &ref.m_q, sizeof(float)), 0);

	for (u32 k = 0; k < ref.itail; ++k)
		EXPECT_EQ(actual.idx[k], ref.idx[k]) << "Mismatch in index at " << k;

	for (u32 slot = 0; slot < ref.tail; ++slot)
	{
		EXPECT_EQ(std::memcmp(&actual.vtx[slot], &ref.vtx[slot], sizeof(GSVertex)), 0)
			<< "Mismatch in vertex buffer at slot " << slot << " in [0, tail)";
	}

	for (u32 slot = ref.tail; slot < kTestMaxVertices; ++slot)
	{
		if (slot < pre_head0 || slot >= pre_tail0)
		{
			EXPECT_TRUE(IsSentinelVertex(actual.vtx[slot]))
				<< "Slot " << slot << " >= tail was unexpectedly modified";
		}
	}
}

static GSChunkedKick::PlannedChunk RunPlanAndConvert(
	const GIFPackedReg* r,
	u32 count,
	const u64 accept[2],
	const GSVector4i& xyof,
	GSChunkedKick::DepthLimitMode depth_limit,
	StripStateBuffer& state,
	bool convert_trailing = true)
{
	const GSChunkedKick::PlannedChunk plan = GSChunkedKick::PlanTriangleStrip(
		accept, count, state.head, state.tail, state.next, state.itail, state.idx, convert_trailing);

	GSChunkedKick::ConvertTriangleStripChunk(
		r, count, plan, state.m_v.UV, xyof, depth_limit,
		state.vtx, state.xy, state.xy_tail, state.m_v, state.m_q);

	state.head = plan.head;
	state.tail = plan.tail;
	state.next = plan.next;
	state.itail = plan.itail;
	return plan;
}

static void RunChunkedPacketDriver(
	const GIFPackedReg* r,
	u32 total_count,
	const GSVector4i& xyof,
	const GSVector4i& scissor,
	bool scissor_invalid,
	bool nativeres,
	GSChunkedKick::DepthLimitMode depth_limit,
	StripStateBuffer& state)
{
	u32 remaining = total_count;
	const GIFPackedReg* cur_r = r;

	const GIFPackedReg* deferred_r = nullptr;
	u32 deferred_count = 0;
	u32 deferred_slot = 0;

	const auto flush_deferred_trailing = [&]()
	{
		if (deferred_count > 0 && state.next < state.head)
		{
			const u32 gap = state.head - state.next;
			const u32 to_convert = std::min<u32>(deferred_count, gap);
			GSChunkedKick::ConvertVertexRange(
				deferred_r, 0, to_convert, deferred_slot,
				state.m_v.UV, depth_limit, state.vtx);
		}
		deferred_r = nullptr;
		deferred_count = 0;
		deferred_slot = 0;
	};

	while (remaining > 0)
	{
		const u32 chunk_count = std::min<u32>(remaining, GSChunkedKick::kMaxChunkVertices);
		if (remaining < 6 || chunk_count < 6)
		{
			flush_deferred_trailing();
			const u32 pending = (state.tail >= state.head) ? (state.tail - state.head) : 0;
			const u32 carried_count = std::min<u32>(pending, 2);
			GSVector4i carried_xy[2];
			if (carried_count == 1)
			{
				carried_xy[0] = state.xy[(state.xy_tail - 1) & 3];
			}
			else if (carried_count == 2)
			{
				carried_xy[0] = state.xy[(state.xy_tail - 2) & 3];
				carried_xy[1] = state.xy[(state.xy_tail - 1) & 3];
			}
			const GSChunkedKick::DecidedChunk decided = GSChunkedKick::DecideTriangleStripPacked(
				cur_r, 1, carried_xy, carried_count, xyof, scissor, scissor_invalid, nativeres);
			ReferencePerVertexStripChunk(cur_r, 1, decided.accept, xyof, depth_limit, state);
			cur_r += 3;
			remaining--;
			continue;
		}

		const u32 pending = (state.tail >= state.head) ? (state.tail - state.head) : 0;
		const u32 carried_count = std::min<u32>(pending, 2);
		GSVector4i carried_xy[2];
		if (carried_count == 1)
		{
			carried_xy[0] = state.xy[(state.xy_tail - 1) & 3];
		}
		else if (carried_count == 2)
		{
			carried_xy[0] = state.xy[(state.xy_tail - 2) & 3];
			carried_xy[1] = state.xy[(state.xy_tail - 1) & 3];
		}

		const GSChunkedKick::DecidedChunk decided = GSChunkedKick::DecideTriangleStripPacked(
			cur_r, chunk_count, carried_xy, carried_count, xyof, scissor, scissor_invalid, nativeres);

		const bool is_last_chunk = (chunk_count == remaining);
		const GSChunkedKick::PlannedChunk planned = RunPlanAndConvert(
			cur_r, chunk_count, decided.accept, xyof, depth_limit, state, is_last_chunk);

		if (decided.accepted_count > 0)
		{
			deferred_r = nullptr;
			deferred_count = 0;
			deferred_slot = 0;
		}

		if (is_last_chunk)
		{
			flush_deferred_trailing();
		}
		else
		{
			const u32 chunk_trailing = planned.trailing_to - planned.trailing_from;
			if (chunk_trailing > 0)
			{
				if (deferred_count == 0)
				{
					deferred_r = cur_r + 3 * planned.trailing_from;
					deferred_count = chunk_trailing;
					deferred_slot = planned.trailing_slot;
				}
				else
				{
					deferred_count += chunk_trailing;
				}
			}
		}

		cur_r += 3 * chunk_count;
		remaining -= chunk_count;
	}

	flush_deferred_trailing();
}

} // namespace

TEST(GSKickChunkedTests, PlanAndConvertAllAccepted)
{
	constexpr u32 count = 10;
	GIFPackedReg r[3 * count];
	for (u32 i = 0; i < count; ++i)
		FillSyntheticPackedVertex(&r[3 * i], i, static_cast<u16>((100 + i * 20) << 4), static_cast<u16>((100 + (i & 1) * 60) << 4));

	u64 accept[2] = {0, 0};
	for (u32 i = 2; i < count; ++i)
		accept[0] |= (1ULL << i);

	const GSVector4i xyof(16, 32, 16, 32);
	StripStateBuffer actual;
	StripStateBuffer ref;
	actual.InitSentinel();
	ref.InitSentinel();

	const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
		r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
	ReferencePerVertexStripChunk(
		r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

	CompareChunkStateAgainstReference(actual, ref);
	EXPECT_EQ(plan.job_count, 1u);
	EXPECT_EQ(plan.jobs[0].from, 0u);
	EXPECT_EQ(plan.jobs[0].to, count);
	EXPECT_EQ(plan.jobs[0].slot, 0u);
}

TEST(GSKickChunkedTests, PlanAndConvertAllRejected)
{
	constexpr u32 count = 12;
	GIFPackedReg r[3 * count];
	for (u32 i = 0; i < count; ++i)
		FillSyntheticPackedVertex(&r[3 * i], i, static_cast<u16>((500 + i * 10) << 4), static_cast<u16>((500 + i * 10) << 4), 1);

	const u64 accept[2] = {0, 0};
	const GSVector4i xyof = GSVector4i::zero();
	StripStateBuffer actual;
	StripStateBuffer ref;
	actual.InitSentinel();
	ref.InitSentinel();

	const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
		r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
	ReferencePerVertexStripChunk(
		r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

	CompareChunkStateAgainstReference(actual, ref);
	EXPECT_EQ(actual.itail, 0u);
	EXPECT_EQ(actual.head, count - 2);
	EXPECT_EQ(actual.tail, count);
	EXPECT_EQ(actual.next, 0u);

	// At packet end (convert_trailing = true), all [0, tail) vertices must be converted
	// to match the per-vertex reference byte-for-byte.
	EXPECT_EQ(plan.job_count, 1u);
	EXPECT_EQ(plan.jobs[0].from, 0u);
	EXPECT_EQ(plan.jobs[0].to, count);
	EXPECT_EQ(plan.jobs[0].slot, 0u);
	for (u32 slot = count; slot < count + 4; ++slot)
		EXPECT_TRUE(IsSentinelVertex(actual.vtx[slot])) << "Slot " << slot << " >= tail was unexpectedly written";
}

TEST(GSKickChunkedTests, PlanAndConvertAlternating)
{
	// Test both stride-2 alternating (2, 4, 6, 8, 10) and stride-4 alternating (2, 6, 10, 14)
	// where 3 consecutive skips between draws trigger compaction and leave interior vertices unconverted.
	for (int mode = 0; mode < 2; ++mode)
	{
		constexpr u32 count = 16;
		GIFPackedReg r[3 * count];
		for (u32 i = 0; i < count; ++i)
			FillSyntheticPackedVertex(&r[3 * i], i, static_cast<u16>((100 + i * 15) << 4), static_cast<u16>((100 + (i & 1) * 50) << 4));

		u64 accept[2] = {0, 0};
		for (u32 i = 2; i < count; ++i)
		{
			const bool draw = (mode == 0) ? ((i & 1) == 0) : (((i - 2) % 4) == 0);
			if (draw)
				accept[0] |= (1ULL << i);
		}

		const GSVector4i xyof = GSVector4i::zero();
		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		RunPlanAndConvert(r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
		ReferencePerVertexStripChunk(r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

		CompareChunkStateAgainstReference(actual, ref);
	}
}

TEST(GSKickChunkedTests, PlanAndConvertProperty1Pattern)
{
	// Property 1 pattern: 5 primitives with accept pattern: drawn, skipped, skipped, drawn, drawn
	// (7 vertices: v0, v1 non-completing; v2 drawn; v3 skipped; v4 skipped; v5 drawn; v6 drawn).
	{
		constexpr u32 count = 7;
		GIFPackedReg r[3 * count];
		for (u32 i = 0; i < count; ++i)
			FillSyntheticPackedVertex(&r[3 * i], i, static_cast<u16>((100 + i * 20) << 4), static_cast<u16>((100 + (i & 1) * 50) << 4));

		const u64 accept[2] = {(1ULL << 2) | (1ULL << 5) | (1ULL << 6), 0};
		const GSVector4i xyof = GSVector4i::zero();

		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		RunPlanAndConvert(r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
		ReferencePerVertexStripChunk(r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

		CompareChunkStateAgainstReference(actual, ref);
		ASSERT_EQ(actual.itail, 9u);
		EXPECT_EQ(actual.idx[0], 0u);
		EXPECT_EQ(actual.idx[1], 1u);
		EXPECT_EQ(actual.idx[2], 2u);
		EXPECT_EQ(actual.idx[3], 3u);
		EXPECT_EQ(actual.idx[4], 4u);
		EXPECT_EQ(actual.idx[5], 5u);
		EXPECT_EQ(actual.idx[6], 4u);
		EXPECT_EQ(actual.idx[7], 5u);
		EXPECT_EQ(actual.idx[8], 6u);
		EXPECT_EQ(actual.head, 5u);
		EXPECT_EQ(actual.tail, 7u);
		EXPECT_EQ(actual.next, 7u);
	}

	// Extended pattern with 3 skips after a draw (drawn, skipped, skipped, skipped, drawn, drawn)
	// which triggers next < head compaction and skips converting vertex 3.
	{
		constexpr u32 count = 8;
		GIFPackedReg r[3 * count];
		for (u32 i = 0; i < count; ++i)
			FillSyntheticPackedVertex(&r[3 * i], i, static_cast<u16>((100 + i * 20) << 4), static_cast<u16>((100 + (i & 1) * 50) << 4));

		const u64 accept[2] = {(1ULL << 2) | (1ULL << 6) | (1ULL << 7), 0};
		const GSVector4i xyof = GSVector4i::zero();

		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
		ReferencePerVertexStripChunk(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

		CompareChunkStateAgainstReference(actual, ref);
		EXPECT_EQ(plan.job_count, 2u);
		EXPECT_EQ(plan.jobs[0].from, 0u);
		EXPECT_EQ(plan.jobs[0].to, 3u);
		EXPECT_EQ(plan.jobs[0].slot, 0u);
		EXPECT_EQ(plan.jobs[1].from, 4u);
		EXPECT_EQ(plan.jobs[1].to, 8u);
		EXPECT_EQ(plan.jobs[1].slot, 3u);
		EXPECT_TRUE(IsSentinelVertex(actual.vtx[7]));
	}
}

TEST(GSKickChunkedTests, PlanAndConvertCarryIn012Vertices)
{
	const GSVector4i xyof = GSVector4i::zero();

	// Carry-in = 0, 1, 2 both without compaction (next0 == head0) and with compaction (next0 < head0)
	for (u32 carry_in = 0; carry_in <= 2; ++carry_in)
	{
		for (u32 gap = 0; gap <= 3; ++gap)
		{
			for (u32 first_accept = 0; first_accept <= 3; ++first_accept)
			{
				constexpr u32 count = 8;
				GIFPackedReg r[3 * count];
				for (u32 i = 0; i < count; ++i)
					FillSyntheticPackedVertex(&r[3 * i], i + 10, static_cast<u16>((100 + i * 20) << 4), static_cast<u16>((100 + (i & 1) * 50) << 4));

				const u32 next0 = 4;
				const u32 head0 = next0 + gap;
				const u32 tail0 = head0 + carry_in;
				const u32 itail0 = 6;

				u64 accept[2] = {0, 0};
				for (u32 i = first_accept; i < count; ++i)
				{
					if (carry_in + i + 1 >= 3 && ((i - first_accept) % 2 == 0))
						accept[0] |= (1ULL << i);
				}

				StripStateBuffer actual;
				StripStateBuffer ref;
				actual.InitSentinel(head0, tail0, next0, itail0, tail0);
				ref.InitSentinel(head0, tail0, next0, itail0, tail0);

				// Populate pre-existing indices and carry-in vertices identically
				for (u32 k = 0; k < itail0; ++k)
				{
					actual.idx[k] = static_cast<u16>(k % next0);
					ref.idx[k] = static_cast<u16>(k % next0);
				}
				for (u32 c = 0; c < carry_in; ++c)
				{
					GIFPackedReg pre_r[3];
					FillSyntheticPackedVertex(pre_r, c + 1, static_cast<u16>((50 + c * 20) << 4), static_cast<u16>((60 + c * 30) << 4));
					GSChunkedKick::ConvertPackedSTQRGBAXYZF2Vertex(
						pre_r, actual.m_v.UV, GSChunkedKick::DepthLimitMode::Disabled, actual.vtx[head0 + c]);
					ref.vtx[head0 + c] = actual.vtx[head0 + c];
				}

				RunPlanAndConvert(r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
				ReferencePerVertexStripChunk(r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

				CompareChunkStateAgainstReference(actual, ref, head0, tail0);
			}
		}
	}
}

TEST(GSKickChunkedTests, PlanAndConvertFull128VertexChunk)
{
	constexpr u32 count = 128;
	GIFPackedReg r[3 * count];
	const GSVector4i scissor(50 << 4, 50 << 4, 2000 << 4, 2000 << 4);
	const GSVector4i xyof = GSVector4i::zero();

	for (u32 i = 0; i < count; ++i)
	{
		u16 x = static_cast<u16>((100 + (i * 10)) << 4);
		u16 y = static_cast<u16>((100 + ((i & 1) ? 80 : 0)) << 4);
		u32 skip = 0;

		if (i == 30)
		{
			x = static_cast<u16>((100 + ((i - 1) * 10)) << 4);
			y = static_cast<u16>((100 + (((i - 1) & 1) ? 80 : 0)) << 4);
		}
		if (i >= 50 && i <= 58)
		{
			skip = 1;
		}
		if (i >= 100 && i <= 107)
		{
			x = static_cast<u16>(3000 << 4);
			y = static_cast<u16>(3000 << 4);
		}
		FillSyntheticPackedVertex(&r[3 * i], i, x, y, skip);
	}

	const GSChunkedKick::DecidedChunk decided = GSChunkedKick::DecideTriangleStripPacked(
		r, count, nullptr, 0, xyof, scissor, false, true);

	StripStateBuffer actual;
	StripStateBuffer ref;
	actual.InitSentinel();
	ref.InitSentinel();

	RunPlanAndConvert(r, count, decided.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
	ReferencePerVertexStripChunk(r, count, decided.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

	CompareChunkStateAgainstReference(actual, ref);
}

TEST(GSKickChunkedTests, PlanAndConvertTwoConsecutiveChunksWithSeamRejections)
{
	constexpr u32 chunk1_count = 128;
	constexpr u32 chunk2_count = 64;
	constexpr u32 total_count = chunk1_count + chunk2_count;

	GIFPackedReg r[3 * total_count];
	const GSVector4i scissor(0, 0, 2000 << 4, 2000 << 4);
	const GSVector4i xyof(10 << 4, 20 << 4, 10 << 4, 20 << 4);

	for (u32 i = 0; i < total_count; ++i)
	{
		const u16 x = static_cast<u16>((100 + (i * 8)) << 4);
		const u16 y = static_cast<u16>((100 + ((i & 1) ? 60 : 0)) << 4);
		// Reject primitives across the seam [124..131] and another interior range [20..25]
		const u32 skip = ((i >= 20 && i <= 25) || (i >= 124 && i <= 131)) ? 1u : 0u;
		FillSyntheticPackedVertex(&r[3 * i], i, x, y, skip);
	}

	StripStateBuffer actual;
	StripStateBuffer ref;
	actual.InitSentinel();
	ref.InitSentinel();

	// Chunk 1
	const GSChunkedKick::DecidedChunk decided1 = GSChunkedKick::DecideTriangleStripPacked(
		r, chunk1_count, nullptr, 0, xyof, scissor, false, true);
	RunPlanAndConvert(r, chunk1_count, decided1.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
	ReferencePerVertexStripChunk(r, chunk1_count, decided1.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);
	CompareChunkStateAgainstReference(actual, ref);

	// Chunk 2 (carrying over pending strip state across the seam)
	const u32 chunk2_head0 = actual.head;
	const u32 chunk2_tail0 = actual.tail;
	const u32 pending = actual.tail - actual.head;
	const u32 carried_count = std::min<u32>(pending, 2);
	GSVector4i carried_xy[2];
	if (carried_count == 1)
	{
		carried_xy[0] = actual.xy[(actual.xy_tail - 1) & 3];
	}
	else if (carried_count == 2)
	{
		carried_xy[0] = actual.xy[(actual.xy_tail - 2) & 3];
		carried_xy[1] = actual.xy[(actual.xy_tail - 1) & 3];
	}

	const GSChunkedKick::DecidedChunk decided2 = GSChunkedKick::DecideTriangleStripPacked(
		r + 3 * chunk1_count, chunk2_count, carried_xy, carried_count, xyof, scissor, false, true);
	RunPlanAndConvert(r + 3 * chunk1_count, chunk2_count, decided2.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
	ReferencePerVertexStripChunk(r + 3 * chunk1_count, chunk2_count, decided2.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);
	CompareChunkStateAgainstReference(actual, ref, chunk2_head0, chunk2_tail0);
}

TEST(GSKickChunkedTests, UnreferencedInteriorSlotsUntouchedAndVertexConversionEdgeCases)
{
	// 12 vertices where only primitive at i=2 (v0,v1,v2) and primitive at i=9 (v7,v8,v9) are accepted,
	// and v10, v11 are skipped at the end (carry-out v10, v11).
	// Unreferenced interior vertices v3, v4, v5, v6 must NEVER be converted!
	// Specifically:
	// - Prim at i=2 writes slots 0, 1, 2 (next=3, head=1)
	// - Skips at i=3..8 advance head to 7 (next=3)
	// - Prim at i=9 compacts (v7, v8, v9) to slots 3, 4, 5 (next=6, head=4, tail=6)
	// - Skips at i=10, 11 write carry-out (v10, v11) to slots 6, 7 (head=6, tail=8, next=6)
	// Slots >= 8 were written in the old per-vertex path before compaction, but in the chunked path
	// must remain untouched (sentinel-filled)!
	constexpr u32 count = 12;
	GIFPackedReg r[3 * count];

	for (u32 i = 0; i < count; ++i)
	{
		float q = 1.0f;
		if (i == 0)
			q = +0.0f; // +0.0f must be replaced by FLT_MIN in vertex buffer, while m_q gets raw STQ.Q if last
		else if (i == 1)
			q = -0.0f; // -0.0f has sign bit 0x80000000 != 0, so it is NOT replaced by FLT_MIN
		else if (i == count - 1)
			q = +0.0f; // Last vertex has raw Q = +0.0f

		FillSyntheticPackedVertex(
			&r[3 * i], i,
			static_cast<u16>((100 + i * 20) << 4),
			static_cast<u16>((100 + (i & 1) * 50) << 4),
			0, 1.5f, 2.5f, q, 0x00ABCDEFu, 0x7Fu);
	}

	const u64 accept[2] = {(1ULL << 2) | (1ULL << 9), 0};
	const GSVector4i xyof(32, 48, 32, 48);

	for (GSChunkedKick::DepthLimitMode mode : {
		GSChunkedKick::DepthLimitMode::Disabled,
		GSChunkedKick::DepthLimitMode::PrioritizeUpper,
		GSChunkedKick::DepthLimitMode::PrioritizeLower})
	{
		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(r, count, accept, xyof, mode, actual);
		ReferencePerVertexStripChunk(r, count, accept, xyof, mode, ref);

		CompareChunkStateAgainstReference(actual, ref);

		// Verify unreferenced vertices v3..v6 were omitted from ConvertJobs
		EXPECT_EQ(plan.job_count, 2u);
		EXPECT_EQ(plan.jobs[0].from, 0u);
		EXPECT_EQ(plan.jobs[0].to, 3u);
		EXPECT_EQ(plan.jobs[0].slot, 0u);
		EXPECT_EQ(plan.jobs[1].from, 7u);
		EXPECT_EQ(plan.jobs[1].to, 12u);
		EXPECT_EQ(plan.jobs[1].slot, 3u);

		// Slots >= 8 were touched by the old per-vertex path before compaction,
		// but must remain sentinel-filled in the chunked path.
		for (u32 slot = 8; slot < 16; ++slot)
			EXPECT_TRUE(IsSentinelVertex(actual.vtx[slot])) << "Slot " << slot << " should be untouched";

		// Verify Q == +0.0f became FLT_MIN in vtx[0].RGBAQ.Q, while m_q holds raw +0.0f from the last vertex
		const float expected_flt_min = FLT_MIN;
		EXPECT_EQ(std::memcmp(&actual.vtx[0].RGBAQ.Q, &expected_flt_min, sizeof(float)), 0);
		const float expected_zero = 0.0f;
		EXPECT_EQ(std::memcmp(&actual.m_q, &expected_zero, sizeof(float)), 0);
	}
}

TEST(GSKickChunkedTests, UnreferencedOverwrittenAndNonOverwrittenSlots)
{
	const GSVector4i xyof = GSVector4i::zero();

	// Case 1: Unreferenced vertices whose old-path slots ARE overwritten by compaction (3+ skips between draws),
	// and trailing skipped vertices at packet end (which ARE converted into [next, tail) so [0, tail) is identical).
	// Count = 10, accepted at i = 2 and i = 6, with 3 skips at i = 7, 8, 9 at packet end:
	// - i = 2 drawn: v0, v1, v2 -> slots 0, 1, 2 (head = 1, tail = 3, next = 3)
	// - i = 3, 4, 5 skipped: old path writes v3 -> slot 3, v4 -> slot 4, v5 -> slot 5 (head = 4, tail = 6, next = 3)
	// - i = 6 drawn: next < head (3 < 4) compacts v4, v5, v6 -> slots 3, 4, 5 (head = 4, tail = 6, next = 6).
	//   Here v3 is never converted in the chunked path because its slot 3 is compacted over by v4.
	// - i = 7, 8, 9 skipped at packet end: converted into [next, tail) = [6, 9) so [0, tail) matches reference.
	{
		constexpr u32 count = 10;
		GIFPackedReg r[3 * count];
		for (u32 i = 0; i < count; ++i)
			FillSyntheticPackedVertex(&r[3 * i], i + 100, static_cast<u16>((100 + i * 20) << 4), static_cast<u16>((100 + (i & 1) * 50) << 4));

		const u64 accept[2] = {(1ULL << 2) | (1ULL << 6), 0};
		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
		ReferencePerVertexStripChunk(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

		CompareChunkStateAgainstReference(actual, ref);

		EXPECT_EQ(actual.head, 7u);
		EXPECT_EQ(actual.tail, 9u);
		EXPECT_EQ(actual.next, 6u);
		EXPECT_EQ(actual.itail, 6u);

		// Slot 3 (written by unreferenced v3 in old path, then overwritten by compacted v4) matches v4 in both paths.
		GSVertex expected_v4 = {};
		u32 dummy_uv = 0;
		GSChunkedKick::ConvertPackedSTQRGBAXYZF2Vertex(
			&r[3 * 4], dummy_uv, GSChunkedKick::DepthLimitMode::Disabled, expected_v4);
		EXPECT_EQ(actual.vtx[3].XYZ.Z, expected_v4.XYZ.Z);
		EXPECT_EQ(std::memcmp(&actual.vtx[3], &ref.vtx[3], sizeof(GSVertex)), 0);

		// Slot 6 (v7 in [next, head)) is converted at packet end, matching ref.vtx[6].
		EXPECT_FALSE(IsSentinelVertex(ref.vtx[6]));
		EXPECT_FALSE(IsSentinelVertex(actual.vtx[6]));
		EXPECT_EQ(std::memcmp(&actual.vtx[6], &ref.vtx[6], sizeof(GSVertex)), 0);

		// Only v3 is omitted; jobs are [0, 3) -> slot 0 and [4, 10) -> slot 3.
		EXPECT_EQ(plan.job_count, 2u);
		EXPECT_EQ(plan.jobs[0].from, 0u);
		EXPECT_EQ(plan.jobs[0].to, 3u);
		EXPECT_EQ(plan.jobs[0].slot, 0u);
		EXPECT_EQ(plan.jobs[1].from, 4u);
		EXPECT_EQ(plan.jobs[1].to, 10u);
		EXPECT_EQ(plan.jobs[1].slot, 3u);
	}

	// Case 2: Skips before the first draw starting from head0 == next0 == 0.
	// Count = 6, accepted only at i = 5 (3 skips at i = 2, 3, 4 before first draw at i = 5):
	// - Old path writes v0..v5 to slots 0..5, then at i = 5 (next=0 < head=3) compacts (v3, v4, v5) into slots 0, 1, 2
	//   (head = 1, tail = 3, next = 3).
	// - Unreferenced v0, v1, v2 at slots 0, 1, 2 are overwritten by compacted (v3, v4, v5) in the old path and directly
	//   written by (v3, v4, v5) in the chunked path.
	// - Pre-compaction slots 3, 4, 5 (written by v3, v4, v5 before compaction in old path) are >= tail (tail = 3):
	//   old path leaves stale copies of v3, v4, v5 in slots 3, 4, 5, while chunked path leaves sentinels.
	{
		constexpr u32 count = 6;
		GIFPackedReg r[3 * count];
		for (u32 i = 0; i < count; ++i)
			FillSyntheticPackedVertex(&r[3 * i], i + 200, static_cast<u16>((100 + i * 20) << 4), static_cast<u16>((100 + (i & 1) * 50) << 4));

		const u64 accept[2] = {(1ULL << 5), 0};
		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
		ReferencePerVertexStripChunk(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

		CompareChunkStateAgainstReference(actual, ref);

		EXPECT_EQ(actual.head, 1u);
		EXPECT_EQ(actual.tail, 3u);
		EXPECT_EQ(actual.next, 3u);
		EXPECT_EQ(actual.itail, 3u);

		EXPECT_EQ(plan.job_count, 1u);
		EXPECT_EQ(plan.jobs[0].from, 3u);
		EXPECT_EQ(plan.jobs[0].to, 6u);
		EXPECT_EQ(plan.jobs[0].slot, 0u);

		for (u32 slot = 3; slot < 6; ++slot)
		{
			EXPECT_FALSE(IsSentinelVertex(ref.vtx[slot]));
			EXPECT_TRUE(IsSentinelVertex(actual.vtx[slot]));
		}
	}
}

TEST(GSKickChunkedTests, PacketEndingIn123And5SkippedPrimitivesAfterDraw)
{
	const GSVector4i xyof = GSVector4i::zero();

	for (u32 trailing_skips : {1u, 2u, 3u, 5u})
	{
		const u32 count = 4 + trailing_skips;
		GIFPackedReg r[3 * 16];
		for (u32 i = 0; i < count; ++i)
			FillSyntheticPackedVertex(&r[3 * i], i + 300, static_cast<u16>((100 + i * 20) << 4), static_cast<u16>((100 + (i & 1) * 50) << 4));

		// Accept primitives at i = 2 and i = 3, then skip the remaining trailing_skips primitives
		const u64 accept[2] = {(1ULL << 2) | (1ULL << 3), 0};

		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
		ReferencePerVertexStripChunk(
			r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

		CompareChunkStateAgainstReference(actual, ref);
		EXPECT_EQ(actual.next, 4u);
		EXPECT_EQ(actual.head, 2u + trailing_skips);
		EXPECT_EQ(actual.tail, 4u + trailing_skips);
		EXPECT_EQ(plan.job_count, 1u);
		EXPECT_EQ(plan.jobs[0].from, 0u);
		EXPECT_EQ(plan.jobs[0].to, count);
		EXPECT_EQ(plan.jobs[0].slot, 0u);
	}
}

TEST(GSKickChunkedTests, PacketWithNoDrawnPrimitiveStartingFromHeadEqualsNext)
{
	const GSVector4i xyof = GSVector4i::zero();

	for (u32 base_slot : {0u, 6u})
	{
		for (u32 carry_in : {0u, 1u, 2u})
		{
			constexpr u32 count = 10;
			GIFPackedReg r[3 * count];
			for (u32 i = 0; i < count; ++i)
				FillSyntheticPackedVertex(&r[3 * i], i + 400, static_cast<u16>((200 + i * 10) << 4), static_cast<u16>((200 + i * 10) << 4), 1);

			const u64 accept[2] = {0, 0};
			const u32 head0 = base_slot;
			const u32 next0 = base_slot;
			const u32 tail0 = base_slot + carry_in;
			const u32 itail0 = (base_slot > 0) ? 6u : 0u;

			StripStateBuffer actual;
			StripStateBuffer ref;
			actual.InitSentinel(head0, tail0, next0, itail0, tail0);
			ref.InitSentinel(head0, tail0, next0, itail0, tail0);

			for (u32 s = 0; s < tail0; ++s)
			{
				GIFPackedReg pre_r[3];
				FillSyntheticPackedVertex(pre_r, s + 1, static_cast<u16>((50 + s * 10) << 4), static_cast<u16>((60 + s * 10) << 4));
				GSChunkedKick::ConvertPackedSTQRGBAXYZF2Vertex(
					pre_r, actual.m_v.UV, GSChunkedKick::DepthLimitMode::Disabled, actual.vtx[s]);
				ref.vtx[s] = actual.vtx[s];
			}
			for (u32 k = 0; k < itail0; ++k)
			{
				actual.idx[k] = static_cast<u16>(k % base_slot);
				ref.idx[k] = static_cast<u16>(k % base_slot);
			}

			const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
				r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
			ReferencePerVertexStripChunk(
				r, count, accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

			CompareChunkStateAgainstReference(actual, ref, 0, tail0);
			EXPECT_EQ(actual.tail, tail0 + count);
			EXPECT_EQ(actual.next, next0);
			EXPECT_EQ(plan.job_count, 1u);
			EXPECT_EQ(plan.jobs[0].from, 0u);
			EXPECT_EQ(plan.jobs[0].to, count);
			EXPECT_EQ(plan.jobs[0].slot, tail0);
		}
	}
}

TEST(GSKickChunkedTests, ThreeOrMoreSkipsThenDrawInsideOneChunkAndAcrossChunkSeam)
{
	const GSVector4i scissor(0, 0, 2000 << 4, 2000 << 4);
	const GSVector4i xyof = GSVector4i::zero();

	// Part A: 3, 4, and 6 skips then a draw inside one chunk
	for (u32 num_skips : {3u, 4u, 6u})
	{
		const u32 count = 4 + num_skips + 2;
		GIFPackedReg r[3 * 16];
		for (u32 i = 0; i < count; ++i)
		{
			const u16 x = static_cast<u16>((100 + i * 20) << 4);
			const u16 y = static_cast<u16>((100 + ((i & 1) ? 60 : 0)) << 4);
			const u32 skip = (i >= 4 && i < 4 + num_skips) ? 1u : 0u;
			FillSyntheticPackedVertex(&r[3 * i], i + 500, x, y, skip);
		}

		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		const GSChunkedKick::DecidedChunk decided = GSChunkedKick::DecideTriangleStripPacked(
			r, count, nullptr, 0, xyof, scissor, false, true);
		const GSChunkedKick::PlannedChunk plan = RunPlanAndConvert(
			r, count, decided.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, actual);
		ReferencePerVertexStripChunk(
			r, count, decided.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);

		CompareChunkStateAgainstReference(actual, ref);

		// Exactly (num_skips - 2) vertices are compacted over and never converted
		u32 total_converted = 0;
		for (u32 j = 0; j < plan.job_count; ++j)
			total_converted += (plan.jobs[j].to - plan.jobs[j].from);
		EXPECT_EQ(total_converted, count - (num_skips - 2));
	}

	// Part B: 3+ skips starting in chunk 1 (indices 123..127 = 5 skips) and continuing in chunk 2
	// (indices 128..129 = 2 skips), followed by drawn primitives at 130..135.
	// Chunk 1 defers its trailing skipped vertices; chunk 2's draw at 130 compacts over them!
	{
		constexpr u32 total_count = 136;
		GIFPackedReg r[3 * total_count];
		for (u32 i = 0; i < total_count; ++i)
		{
			const u16 x = static_cast<u16>((100 + i * 8) << 4);
			const u16 y = static_cast<u16>((100 + ((i & 1) ? 60 : 0)) << 4);
			const u32 skip = (i >= 123 && i <= 129) ? 1u : 0u;
			FillSyntheticPackedVertex(&r[3 * i], i + 600, x, y, skip);
		}

		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		RunChunkedPacketDriver(r, total_count, xyof, scissor, false, true, GSChunkedKick::DepthLimitMode::Disabled, actual);

		// Reference across the whole packet
		for (u32 i = 0; i < total_count; ++i)
		{
			const u32 pending = (ref.tail >= ref.head) ? (ref.tail - ref.head) : 0;
			const u32 carried_count = std::min<u32>(pending, 2);
			GSVector4i carried_xy[2];
			if (carried_count == 1)
				carried_xy[0] = ref.xy[(ref.xy_tail - 1) & 3];
			else if (carried_count == 2)
			{
				carried_xy[0] = ref.xy[(ref.xy_tail - 2) & 3];
				carried_xy[1] = ref.xy[(ref.xy_tail - 1) & 3];
			}
			const GSChunkedKick::DecidedChunk d = GSChunkedKick::DecideTriangleStripPacked(
				r + 3 * i, 1, carried_xy, carried_count, xyof, scissor, false, true);
			ReferencePerVertexStripChunk(r + 3 * i, 1, d.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);
		}

		CompareChunkStateAgainstReference(actual, ref);
		// Final tail is 136 - (7 - 2) = 131; slots >= 131 were never written by either the chunked path
		// or the reference path (both end at tail 131 because post-compaction draws 131..135
		// overwrote slots 126..130 rather than advancing past 130).
		EXPECT_EQ(actual.tail, 131u);
		for (u32 slot = 131; slot < 136; ++slot)
		{
			EXPECT_TRUE(IsSentinelVertex(ref.vtx[slot]));
			EXPECT_TRUE(IsSentinelVertex(actual.vtx[slot]));
		}
	}
}

TEST(GSKickChunkedTests, TrailingSkippedRunAcrossChunkSeamToPacketEnd)
{
	const GSVector4i scissor(0, 0, 2000 << 4, 2000 << 4);
	const GSVector4i xyof = GSVector4i::zero();

	// Test both when chunk 2 is >= 6 vertices (140 total = 128 + 12) and when chunk 2 < 6 vertices
	// bailing to the per-vertex remainder loop (132 total = 128 + 4).
	for (u32 total_count : {140u, 132u})
	{
		GIFPackedReg r[3 * 144];
		for (u32 i = 0; i < total_count; ++i)
		{
			const u16 x = static_cast<u16>((100 + i * 8) << 4);
			const u16 y = static_cast<u16>((100 + ((i & 1) ? 60 : 0)) << 4);
			// Trailing skipped run starts at index 120 in chunk 1 and continues to packet end in chunk 2
			const u32 skip = (i >= 120) ? 1u : 0u;
			FillSyntheticPackedVertex(&r[3 * i], i + 800, x, y, skip);
		}

		StripStateBuffer actual;
		StripStateBuffer ref;
		actual.InitSentinel();
		ref.InitSentinel();

		RunChunkedPacketDriver(r, total_count, xyof, scissor, false, true, GSChunkedKick::DepthLimitMode::Disabled, actual);

		for (u32 i = 0; i < total_count; ++i)
		{
			const u32 pending = (ref.tail >= ref.head) ? (ref.tail - ref.head) : 0;
			const u32 carried_count = std::min<u32>(pending, 2);
			GSVector4i carried_xy[2];
			if (carried_count == 1)
				carried_xy[0] = ref.xy[(ref.xy_tail - 1) & 3];
			else if (carried_count == 2)
			{
				carried_xy[0] = ref.xy[(ref.xy_tail - 2) & 3];
				carried_xy[1] = ref.xy[(ref.xy_tail - 1) & 3];
			}
			const GSChunkedKick::DecidedChunk d = GSChunkedKick::DecideTriangleStripPacked(
				r + 3 * i, 1, carried_xy, carried_count, xyof, scissor, false, true);
			ReferencePerVertexStripChunk(r + 3 * i, 1, d.accept, xyof, GSChunkedKick::DepthLimitMode::Disabled, ref);
		}

		CompareChunkStateAgainstReference(actual, ref);
		EXPECT_EQ(actual.next, 120u);
		EXPECT_EQ(actual.head, total_count - 2);
		EXPECT_EQ(actual.tail, total_count);
	}
}
