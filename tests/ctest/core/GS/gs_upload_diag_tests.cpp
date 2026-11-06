// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/Renderers/Common/GSUploadDiag.h"

#include <gtest/gtest.h>

TEST(GSUploadDiag, ArmBBudgetAllowsBoundary)
{
	const std::uint64_t budget = GSUploadDiag::kArmBFreshBudgetBytes;
	EXPECT_TRUE(GSUploadDiag::ArmBBudgetAllows(0, budget));
	EXPECT_TRUE(GSUploadDiag::ArmBBudgetAllows(budget, 0));
	EXPECT_TRUE(GSUploadDiag::ArmBBudgetAllows(budget - 1, 1));
	EXPECT_FALSE(GSUploadDiag::ArmBBudgetAllows(budget, 1));
	EXPECT_FALSE(GSUploadDiag::ArmBBudgetAllows(0, budget + 1));
	EXPECT_TRUE(GSUploadDiag::ArmBBudgetAllows(0, 0));
	EXPECT_TRUE(GSUploadDiag::ArmBBudgetAllows(1, 1));
	EXPECT_FALSE(GSUploadDiag::ArmBBudgetAllows(budget, budget));
	EXPECT_FALSE(GSUploadDiag::ArmBBudgetAllows(~std::uint64_t{0}, ~std::uint64_t{0}));
}

TEST(GSUploadDiag, ArmAEligible)
{
	EXPECT_TRUE(GSUploadDiag::ArmAEligible(false));
	EXPECT_FALSE(GSUploadDiag::ArmAEligible(true));
}

TEST(GSUploadDiag, FormatLinePrefixNamesOrderAndValues)
{
	GSUploadDiag::Counters c{};
	c.uploads = 1;
	c.bytes = 2;
	c.uploads_compressed = 3;
	c.uploads_client_mem = 4;
	c.uploads_stream = 5;
	c.uploads_into_busy_dest = 6;
	c.fetches = 7;
	c.fetch_reused = 8;
	c.fetch_reused_busy = 9;
	c.fetch_created = 10;
	c.armb_eligible = 11;
	c.armb_taken = 12;
	c.armb_fresh = 13;
	c.armb_busy_skips = 14;
	c.armb_fallbacks = 15;
	c.armb_fresh_bytes = 16;

	const std::string line = GSUploadDiag::FormatLine(c, 1000, 60);
	EXPECT_EQ(line,
		"GSUPLOAD: ms=1000 vsyncs=60 uploads=1 bytes=2 uploads_compressed=3 "
		"uploads_client_mem=4 uploads_stream=5 uploads_into_busy_dest=6 fetches=7 "
		"fetch_reused=8 fetch_reused_busy=9 fetch_created=10 armb_eligible=11 "
		"armb_taken=12 armb_fresh=13 armb_busy_skips=14 armb_fallbacks=15 armb_fresh_bytes=16");
}

TEST(GSUploadDiag, ResetZeroesEveryField)
{
	GSUploadDiag::Counters c{};
	c.uploads = 1;
	c.bytes = 2;
	c.uploads_compressed = 3;
	c.uploads_client_mem = 4;
	c.uploads_stream = 5;
	c.uploads_into_busy_dest = 6;
	c.fetches = 7;
	c.fetch_reused = 8;
	c.fetch_reused_busy = 9;
	c.fetch_created = 10;
	c.armb_eligible = 11;
	c.armb_taken = 12;
	c.armb_fresh = 13;
	c.armb_busy_skips = 14;
	c.armb_fallbacks = 15;
	c.armb_fresh_bytes = 16;

	GSUploadDiag::Reset(c);

	EXPECT_EQ(c.uploads, 0u);
	EXPECT_EQ(c.bytes, 0u);
	EXPECT_EQ(c.uploads_compressed, 0u);
	EXPECT_EQ(c.uploads_client_mem, 0u);
	EXPECT_EQ(c.uploads_stream, 0u);
	EXPECT_EQ(c.uploads_into_busy_dest, 0u);
	EXPECT_EQ(c.fetches, 0u);
	EXPECT_EQ(c.fetch_reused, 0u);
	EXPECT_EQ(c.fetch_reused_busy, 0u);
	EXPECT_EQ(c.fetch_created, 0u);
	EXPECT_EQ(c.armb_eligible, 0u);
	EXPECT_EQ(c.armb_taken, 0u);
	EXPECT_EQ(c.armb_fresh, 0u);
	EXPECT_EQ(c.armb_busy_skips, 0u);
	EXPECT_EQ(c.armb_fallbacks, 0u);
	EXPECT_EQ(c.armb_fresh_bytes, 0u);
}
