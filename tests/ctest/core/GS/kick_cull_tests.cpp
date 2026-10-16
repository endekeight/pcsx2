// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSKickCull.h"

#include <gtest/gtest.h>

#include <climits>
#include <random>

template <int primclass>
static void CheckRandom(u32 seed, int range, int count)
{
	std::mt19937 rng(seed);
	std::uniform_int_distribution<int> dist(-range, range);
	int failures = 0;

	for (int i = 0; i < count; ++i)
	{
		const GSVector4i bbox(dist(rng), dist(rng), dist(rng), dist(rng));
		const GSVector4i scissor(dist(rng), dist(rng), dist(rng), dist(rng));
		const GSVector4i v0(dist(rng), dist(rng), dist(rng), dist(rng));
		const GSVector4i v1(dist(rng), dist(rng), dist(rng), dist(rng));
		const GSVector4i v2(dist(rng), dist(rng), dist(rng), dist(rng));

		const u32 mask = GSKickCullMask<primclass>(bbox, scissor, v0, v1, v2);
		const u32 ref = GSKickCullReference<primclass>(bbox, scissor, v0, v1, v2);

		if (mask != ref)
		{
			EXPECT_EQ(GSKickCullMask<primclass>(bbox, scissor, v0, v1, v2),
			          GSKickCullReference<primclass>(bbox, scissor, v0, v1, v2))
				<< "case index: " << i;
			if (++failures > 20)
			{
				break;
			}
		}
	}
	EXPECT_EQ(failures, 0);
}

TEST(GSKickCull, MatchesReferenceTriangleSmallRange)
{
	CheckRandom<GS_TRIANGLE_CLASS>(1, 4, 1000000);
}

TEST(GSKickCull, MatchesReferenceSpriteSmallRange)
{
	CheckRandom<GS_SPRITE_CLASS>(2, 4, 1000000);
}

TEST(GSKickCull, MatchesReferenceLineSmallRange)
{
	CheckRandom<GS_LINE_CLASS>(3, 4, 1000000);
}

TEST(GSKickCull, MatchesReferencePointSmallRange)
{
	CheckRandom<GS_POINT_CLASS>(4, 4, 1000000);
}

TEST(GSKickCull, MatchesReferenceTriangleWideRange)
{
	CheckRandom<GS_TRIANGLE_CLASS>(5, 1 << 20, 200000);
}

TEST(GSKickCull, EdgeCases)
{
	const GSVector4i scissor(100, 100, 500, 500);

	// Identical v0 and v1 (degenerate) with a valid box
	{
		const GSVector4i bbox(150, 150, 350, 350);
		const GSVector4i v0(200, 200, 0, 0);
		const GSVector4i v1(200, 200, 0, 0);
		const GSVector4i v2(300, 300, 0, 0);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2));
	}

	// Box with x0 == x1
	{
		const GSVector4i bbox(200, 150, 200, 350);
		const GSVector4i v0(200, 150, 0, 0);
		const GSVector4i v1(200, 250, 0, 0);
		const GSVector4i v2(200, 350, 0, 0);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2));
	}

	// Box fully outside the scissor
	{
		const GSVector4i bbox(600, 600, 800, 800);
		const GSVector4i v0(600, 600, 0, 0);
		const GSVector4i v1(700, 700, 0, 0);
		const GSVector4i v2(800, 800, 0, 0);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2));
	}

	// Box touching the scissor edge (bbox x1 == scissor x0 - 1 and == scissor x0)
	{
		const GSVector4i bbox(50, 150, 99, 350);
		const GSVector4i v0(50, 150, 0, 0);
		const GSVector4i v1(70, 250, 0, 0);
		const GSVector4i v2(99, 350, 0, 0);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2));
	}
	{
		const GSVector4i bbox(50, 150, 100, 350);
		const GSVector4i v0(50, 150, 0, 0);
		const GSVector4i v1(70, 250, 0, 0);
		const GSVector4i v2(100, 350, 0, 0);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox, scissor, v0, v1, v2));
	}

	// INT_MAX / INT_MIN coordinates in the box
	{
		const GSVector4i v0(0, 0, 0, 0);
		const GSVector4i v1(10, 10, 0, 0);
		const GSVector4i v2(20, 20, 0, 0);

		const GSVector4i bbox_max(INT_MAX, INT_MAX, INT_MAX, INT_MAX);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox_max, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox_max, scissor, v0, v1, v2));

		const GSVector4i bbox_min(INT_MIN, INT_MIN, INT_MIN, INT_MIN);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox_min, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox_min, scissor, v0, v1, v2));

		const GSVector4i bbox_mix(INT_MIN, INT_MIN, INT_MAX, INT_MAX);
		EXPECT_EQ(GSKickCullMask<GS_TRIANGLE_CLASS>(bbox_mix, scissor, v0, v1, v2),
		          GSKickCullReference<GS_TRIANGLE_CLASS>(bbox_mix, scissor, v0, v1, v2));
	}
}
