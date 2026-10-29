// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/Renderers/Common/GSTextureCostStats.h"

#include <gtest/gtest.h>

namespace
{
	static std::uint64_t s_cpu_calls = 0;
	static std::uint64_t s_wall_calls = 0;
	static const std::uint64_t* s_cpu_seq = nullptr;
	static std::size_t s_cpu_seq_len = 0;
	static std::size_t s_cpu_seq_idx = 0;
	static const std::uint64_t* s_wall_seq = nullptr;
	static std::size_t s_wall_seq_len = 0;
	static std::size_t s_wall_seq_idx = 0;
	static std::uint64_t s_fixed_cpu = 1000;
	static std::uint64_t s_fixed_wall = 2000;

	void ResetFakeClocks()
	{
		s_cpu_calls = 0;
		s_wall_calls = 0;
		s_cpu_seq = nullptr;
		s_cpu_seq_len = 0;
		s_cpu_seq_idx = 0;
		s_wall_seq = nullptr;
		s_wall_seq_len = 0;
		s_wall_seq_idx = 0;
		s_fixed_cpu = 1000;
		s_fixed_wall = 2000;
	}

	void SetCpuSequence(const std::uint64_t* seq, std::size_t len)
	{
		s_cpu_seq = seq;
		s_cpu_seq_len = len;
		s_cpu_seq_idx = 0;
	}

	void SetWallSequence(const std::uint64_t* seq, std::size_t len)
	{
		s_wall_seq = seq;
		s_wall_seq_len = len;
		s_wall_seq_idx = 0;
	}

	std::uint64_t FakeCpuClock()
	{
		s_cpu_calls++;
		if (s_cpu_seq && s_cpu_seq_idx < s_cpu_seq_len)
			return s_cpu_seq[s_cpu_seq_idx++];
		return s_fixed_cpu;
	}

	std::uint64_t FakeWallClock()
	{
		s_wall_calls++;
		if (s_wall_seq && s_wall_seq_idx < s_wall_seq_len)
			return s_wall_seq[s_wall_seq_idx++];
		return s_fixed_wall;
	}
} // namespace

TEST(GSTextureCostSamplerTest, SamplingIntervalAndNoClocksWhenUnsampled)
{
	GSTextureCost::Totals totals{};
	ResetFakeClocks();
	GSTextureCost::StackTimer timer(&totals, FakeCpuClock, FakeWallClock);

	// 32 outer entries, each with a nested entry
	for (int i = 0; i < 32; ++i)
	{
		timer.Enter(GSTextureCost::Region::Lookup);
		timer.Enter(GSTextureCost::Region::CreateSource);
		timer.Exit(GSTextureCost::Region::CreateSource);
		timer.Exit(GSTextureCost::Region::Lookup);
	}

	// 32 outer entries sample exactly 2 (interval = 16: entry 0 and 16)
	EXPECT_EQ(totals.sampled_outer_calls, 2u);

	// In the 2 sampled outer entries, clock is read 4 times each (outer Enter, inner Enter, inner Exit, outer Exit)
	// 2 * 4 = 8 reads total. All 30 unsampled iterations read zero clocks.
	EXPECT_EQ(s_cpu_calls, 8u);
	EXPECT_EQ(s_wall_calls, 8u);

	// Explicitly verify unsampled nested calls read no clock
	GSTextureCost::Totals totals_single{};
	ResetFakeClocks();
	GSTextureCost::StackTimer timer_single(&totals_single, FakeCpuClock, FakeWallClock);

	// 1st entry: sampled (counter 0 -> 1)
	timer_single.Enter(GSTextureCost::Region::Lookup);
	timer_single.Exit(GSTextureCost::Region::Lookup);
	EXPECT_EQ(totals_single.sampled_outer_calls, 1u);
	EXPECT_EQ(s_cpu_calls, 2u);
	EXPECT_EQ(s_wall_calls, 2u);

	// 2nd entry: unsampled (counter 1 -> 2)
	ResetFakeClocks();
	timer_single.Enter(GSTextureCost::Region::Lookup);
	timer_single.Enter(GSTextureCost::Region::CreateSource);
	timer_single.Enter(GSTextureCost::Region::UploadCopy);
	timer_single.Exit(GSTextureCost::Region::UploadCopy);
	timer_single.Exit(GSTextureCost::Region::CreateSource);
	timer_single.Exit(GSTextureCost::Region::Lookup);

	// Zero clocks read for unsampled outer entry and all its nested entries
	EXPECT_EQ(s_cpu_calls, 0u);
	EXPECT_EQ(s_wall_calls, 0u);
	EXPECT_EQ(totals_single.sampled_outer_calls, 1u);
}

TEST(GSTextureCostSamplerTest, NestingAndExclusiveAccounting)
{
	GSTextureCost::Totals totals{};
	ResetFakeClocks();

	// Timeline:
	// Enter(Lookup):       cpu = 100,  wall = 1000
	// Enter(CreateSource): cpu = 130,  wall = 1050  -> Lookup duration = 30 cpu, 50 wall
	// Enter(UploadCopy):   cpu = 180,  wall = 1150  -> CreateSource duration = 50 cpu, 100 wall
	// Exit(UploadCopy):    cpu = 220,  wall = 1200  -> UploadCopy duration = 40 cpu, 50 wall, resume CreateSource at 220, 1200
	// Exit(CreateSource):  cpu = 250,  wall = 1300  -> CreateSource duration += 30 cpu, 100 wall (total: 80 cpu, 200 wall), resume Lookup at 250, 1300
	// Exit(Lookup):        cpu = 310,  wall = 1400  -> Lookup duration += 60 cpu, 100 wall (total: 90 cpu, 150 wall)
	const std::uint64_t cpu_timeline[] = {100, 130, 180, 220, 250, 310};
	const std::uint64_t wall_timeline[] = {1000, 1050, 1150, 1200, 1300, 1400};

	SetCpuSequence(cpu_timeline, sizeof(cpu_timeline) / sizeof(cpu_timeline[0]));
	SetWallSequence(wall_timeline, sizeof(wall_timeline) / sizeof(wall_timeline[0]));

	GSTextureCost::StackTimer timer(&totals, FakeCpuClock, FakeWallClock);

	timer.Enter(GSTextureCost::Region::Lookup);
	timer.Enter(GSTextureCost::Region::CreateSource);
	timer.Enter(GSTextureCost::Region::UploadCopy);
	timer.Exit(GSTextureCost::Region::UploadCopy);
	timer.Exit(GSTextureCost::Region::CreateSource);
	timer.Exit(GSTextureCost::Region::Lookup);

	EXPECT_EQ(totals.sampled_outer_calls, 1u);
	EXPECT_EQ(totals.clock_anomalies, 0u);
	EXPECT_EQ(totals.depth_overflows, 0u);
	EXPECT_EQ(totals.unbalanced_exits, 0u);

	const auto idx_lookup = static_cast<std::size_t>(GSTextureCost::Region::Lookup);
	const auto idx_createsrc = static_cast<std::size_t>(GSTextureCost::Region::CreateSource);
	const auto idx_uploadcopy = static_cast<std::size_t>(GSTextureCost::Region::UploadCopy);

	EXPECT_EQ(totals.region_cpu_ns[idx_lookup], 90u);
	EXPECT_EQ(totals.region_cpu_ns[idx_createsrc], 80u);
	EXPECT_EQ(totals.region_cpu_ns[idx_uploadcopy], 40u);

	EXPECT_EQ(totals.region_wall_ns[idx_lookup], 150u);
	EXPECT_EQ(totals.region_wall_ns[idx_createsrc], 200u);
	EXPECT_EQ(totals.region_wall_ns[idx_uploadcopy], 50u);

	EXPECT_EQ(totals.region_samples[idx_lookup], 1u);
	EXPECT_EQ(totals.region_samples[idx_createsrc], 1u);
	EXPECT_EQ(totals.region_samples[idx_uploadcopy], 1u);
}

TEST(GSTextureCostSamplerTest, StackOverflowSafety)
{
	GSTextureCost::Totals totals{};
	ResetFakeClocks();

	// 8 levels fit in stack (kMaxDepth = 8).
	// Levels 0..7 enter: 1 initial read + 7 enter reads = 8 reads.
	// 9th and 10th enter: depth overflow (depth 8 and 9), no clock read.
	// 1st and 2nd exit: depth decrements (9 and 8), depth >= kMaxDepth -> no clock read.
	// Remaining 8 exits: 8 clock reads.
	// Total clock reads = 8 + 8 = 16.
	std::uint64_t cpu_timeline[16];
	std::uint64_t wall_timeline[16];
	for (std::size_t i = 0; i < 16; ++i)
	{
		cpu_timeline[i] = 1000 + i * 10;
		wall_timeline[i] = 2000 + i * 20;
	}

	SetCpuSequence(cpu_timeline, 16);
	SetWallSequence(wall_timeline, 16);

	GSTextureCost::StackTimer timer(&totals, FakeCpuClock, FakeWallClock);

	// Push 10 nested entries
	for (int i = 0; i < 10; ++i)
	{
		timer.Enter(static_cast<GSTextureCost::Region>(i % static_cast<int>(GSTextureCost::Region::Count)));
	}

	EXPECT_EQ(totals.depth_overflows, 2u);
	EXPECT_EQ(timer.GetDepth(), 10u);

	// Pop 10 nested entries
	for (int i = 9; i >= 0; --i)
	{
		timer.Exit(static_cast<GSTextureCost::Region>(i % static_cast<int>(GSTextureCost::Region::Count)));
	}

	EXPECT_EQ(timer.GetDepth(), 0u);
	EXPECT_FALSE(timer.IsActive());

	// All 8 tracked levels were exited and counted in region_samples
	std::uint64_t total_samples = 0;
	for (std::size_t i = 0; i < static_cast<std::size_t>(GSTextureCost::Region::Count); ++i)
		total_samples += totals.region_samples[i];
	EXPECT_EQ(total_samples, 8u);

	// Total elapsed time across all 8 levels equals the outer start to outer end
	std::uint64_t total_cpu = 0;
	std::uint64_t total_wall = 0;
	for (std::size_t i = 0; i < static_cast<std::size_t>(GSTextureCost::Region::Count); ++i)
	{
		total_cpu += totals.region_cpu_ns[i];
		total_wall += totals.region_wall_ns[i];
	}
	EXPECT_EQ(total_cpu, cpu_timeline[15] - cpu_timeline[0]);
	EXPECT_EQ(total_wall, wall_timeline[15] - wall_timeline[0]);
}

TEST(GSTextureCostSamplerTest, ClockAnomalyResilience)
{
	GSTextureCost::Totals totals{};
	ResetFakeClocks();

	// Backward clock on CPU: start 500, end 400. Wall normal: start 1000, end 1100.
	const std::uint64_t cpu_jump1[] = {500, 400};
	const std::uint64_t wall_jump1[] = {1000, 1100};

	SetCpuSequence(cpu_jump1, 2);
	SetWallSequence(wall_jump1, 2);

	GSTextureCost::StackTimer timer(&totals, FakeCpuClock, FakeWallClock);
	timer.Enter(GSTextureCost::Region::Lookup);
	timer.Exit(GSTextureCost::Region::Lookup);

	EXPECT_EQ(totals.clock_anomalies, 1u);
	const auto idx_lookup = static_cast<std::size_t>(GSTextureCost::Region::Lookup);
	EXPECT_EQ(totals.region_cpu_ns[idx_lookup], 0u);
	EXPECT_EQ(totals.region_wall_ns[idx_lookup], 100u);

	// Backward clock on Wall: start 1000, end 900. CPU normal: start 100, end 200.
	// Reset timer counter so it samples again
	timer.Reset();
	const std::uint64_t cpu_jump2[] = {100, 200};
	const std::uint64_t wall_jump2[] = {1000, 900};

	SetCpuSequence(cpu_jump2, 2);
	SetWallSequence(wall_jump2, 2);

	timer.Enter(GSTextureCost::Region::Lookup);
	timer.Exit(GSTextureCost::Region::Lookup);

	EXPECT_EQ(totals.clock_anomalies, 2u);
	EXPECT_EQ(totals.region_cpu_ns[idx_lookup], 100u);
	EXPECT_EQ(totals.region_wall_ns[idx_lookup], 100u);
}

TEST(GSTextureCostSamplerTest, ExitAtDepthZeroCountsUnbalancedExits)
{
	GSTextureCost::Totals totals{};
	GSTextureCost::StackTimer timer(&totals);

	EXPECT_EQ(totals.unbalanced_exits, 0u);
	timer.Exit(GSTextureCost::Region::Lookup);
	EXPECT_EQ(totals.unbalanced_exits, 1u);
	timer.Exit(GSTextureCost::Region::CreateSource);
	EXPECT_EQ(totals.unbalanced_exits, 2u);
	EXPECT_EQ(timer.GetDepth(), 0u);
}

TEST(GSTextureCostSamplerTest, RegionSamplesCountedPerRegion)
{
	GSTextureCost::Totals totals{};
	ResetFakeClocks();
	GSTextureCost::StackTimer timer(&totals, FakeCpuClock, FakeWallClock);

	for (std::size_t r = 0; r < static_cast<std::size_t>(GSTextureCost::Region::Count); ++r)
	{
		timer.Reset(); // ensures counter % 16 == 0
		timer.Enter(static_cast<GSTextureCost::Region>(r));
		timer.Exit(static_cast<GSTextureCost::Region>(r));
		EXPECT_EQ(totals.region_samples[r], 1u);
	}
}

TEST(GSTextureCostSamplerTest, ZeroOverheadUnderFlagZero)
{
#if !GS_TEXTURE_COST_STATS
	GS_TEXTURE_COST_SCOPE(Lookup);
	GS_TEXTURE_COST_BEGIN(Lookup);
	GS_TEXTURE_COST_END(Lookup);
	GS_TEXTURE_COST_INC(lookups);
	GS_TEXTURE_COST_ADD(source_flushed_rects, 10);
	SUCCEED();
#endif
}

TEST(GSTextureCostBytesTest, BC1Like)
{
	EXPECT_EQ(GSTextureCost::CompressedUsefulBytes(4, 4, 4, 8), 8u);
	EXPECT_EQ(GSTextureCost::CompressedUsefulBytes(5, 4, 4, 8), 16u);
	EXPECT_EQ(GSTextureCost::CompressedUsefulBytes(4, 5, 4, 8), 16u);
	EXPECT_EQ(GSTextureCost::CompressedUsefulBytes(8, 8, 4, 8), 32u);
}

TEST(GSTextureCostBytesTest, BC3Like)
{
	EXPECT_EQ(GSTextureCost::CompressedUsefulBytes(4, 4, 4, 16), 16u);
}

TEST(GSTextureCostBytesTest, UncompressedLike)
{
	EXPECT_EQ(GSTextureCost::CompressedUsefulBytes(3, 2, 1, 4), 24u);
}

TEST(GSTextureCostBytesTest, PaddedPitchIgnored)
{
	// The source pitch does not enter the useful-byte formula.
	const std::uint32_t width = 4;
	const std::uint32_t height = 4;
	const std::uint32_t block_dim = 4;
	const std::uint32_t bytes_per_block = 8;

	EXPECT_EQ(GSTextureCost::CompressedUsefulBytes(width, height, block_dim, bytes_per_block), 8u);
}

