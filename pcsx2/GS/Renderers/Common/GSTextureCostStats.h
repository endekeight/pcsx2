// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#ifndef GS_TEXTURE_COST_STATS
#define GS_TEXTURE_COST_STATS 0
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>

namespace GSTextureCost
{
	enum class Region : std::size_t
	{
		Lookup = 0,
		CreateSource,
		SourceUpdate,
		Invalidate,
		Palette,
		PreloadMip,
		UploadCopy,
		UploadGL,
		StreamMap,
		Count
	};

	inline constexpr std::array<const char*, static_cast<std::size_t>(Region::Count)> kRegionNames = {
		"Lookup",
		"CreateSource",
		"SourceUpdate",
		"Invalidate",
		"Palette",
		"PreloadMip",
		"UploadCopy",
		"UploadGL",
		"StreamMap"
	};

	struct Totals
	{
		// Lookups
		std::uint64_t lookups = 0;
		std::uint64_t lookups_reused = 0;
		std::uint64_t lookups_created_source = 0;
		std::uint64_t lookups_created_merged = 0;

		// Palette
		std::uint64_t palette_lookups = 0;
		std::uint64_t palette_attaches = 0;

		// Source Updates
		std::uint64_t source_update_work = 0;
		std::uint64_t source_update_early = 0;
		std::uint64_t source_write_calls = 0;
		std::uint64_t source_flush_calls = 0;
		std::uint64_t source_flushed_rects = 0;

		// Preload / Mip
		std::uint64_t preload_level_calls = 0;
		std::uint64_t preload_texture_calls = 0;
		std::uint64_t generate_mipmap_calls = 0;

		// Invalidation
		std::uint64_t inv_videomem_calls = 0;
		std::uint64_t inv_videomemtype_calls = 0;
		std::uint64_t inv_localmem_calls = 0;
		std::uint64_t inv_containedtargets_calls = 0;
		std::uint64_t inv_subtarget_calls = 0;
		std::uint64_t sources_removed = 0;

		// Upload Paths
		std::uint64_t uploads_compressed = 0;
		std::uint64_t uploads_direct = 0;
		std::uint64_t uploads_staged = 0;
		std::uint64_t uploads_map_unmap = 0;

		// Upload Bytes
		std::uint64_t bytes_useful_compressed = 0;
		std::uint64_t bytes_submitted_compressed = 0;
		std::uint64_t bytes_useful_direct = 0;
		std::uint64_t bytes_submitted_direct = 0;
		std::uint64_t bytes_useful_staged = 0;
		std::uint64_t bytes_submitted_staged = 0;
		std::uint64_t bytes_useful_map_unmap = 0;
		std::uint64_t bytes_submitted_map_unmap = 0;

		// Upload Characteristics
		std::uint64_t uploads_height_1 = 0;
		std::uint64_t hist_le_1k = 0;
		std::uint64_t hist_le_16k = 0;
		std::uint64_t hist_le_256k = 0;
		std::uint64_t hist_gt_256k = 0;

		// Clears
		std::uint64_t commit_clears = 0;

		// Stream Buffer Events (Index 0 = TextureUpload, Index 1 = Other)
		std::uint64_t streambuf_map_calls[2]{0, 0};
		std::uint64_t streambuf_wraps_to_zero[2]{0, 0};
		std::uint64_t streambuf_wait_for_sync_calls[2]{0, 0};
		std::uint64_t streambuf_wait_wall_ns[2]{0, 0};

		// Sampled Region Timers
		std::uint64_t region_cpu_ns[static_cast<std::size_t>(Region::Count)]{0};
		std::uint64_t region_wall_ns[static_cast<std::size_t>(Region::Count)]{0};
		std::uint64_t region_samples[static_cast<std::size_t>(Region::Count)]{0};
		std::uint64_t sampled_outer_calls = 0;
		std::uint64_t clock_anomalies = 0;
		std::uint64_t depth_overflows = 0;
		std::uint64_t unbalanced_exits = 0;
	};

	using ClockFn = std::uint64_t (*)();

	inline std::uint64_t ReadCpuNs()
	{
		struct timespec ts;
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
		return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<std::uint64_t>(ts.tv_nsec);
	}

	inline std::uint64_t ReadWallNs()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<std::uint64_t>(ts.tv_nsec);
	}

	class StackTimer
	{
	public:
		static constexpr std::size_t kMaxDepth = 8;
		static constexpr std::uint32_t kSampleInterval = 16;

		struct StackEntry
		{
			Region region;
			std::uint64_t cpu_start;
			std::uint64_t wall_start;
		};

		explicit StackTimer(Totals* totals = nullptr, ClockFn cpu = ReadCpuNs, ClockFn wall = ReadWallNs)
			: m_totals(totals)
			, m_cpu_clock(cpu)
			, m_wall_clock(wall)
		{
		}

		inline void Enter(Region r)
		{
			if (m_depth == 0)
			{
				m_active = (m_counter % kSampleInterval == 0);
				m_counter++;
				if (m_active && m_totals)
					m_totals->sampled_outer_calls++;
			}
			if (m_depth >= kMaxDepth)
			{
				if (m_totals)
					m_totals->depth_overflows++;
				m_depth++;
				return;
			}
			if (m_active)
			{
				const std::uint64_t cpu = m_cpu_clock();
				const std::uint64_t wall = m_wall_clock();
				if (m_depth > 0)
					Charge(m_stack[m_depth - 1], cpu, wall);
				m_stack[m_depth] = {r, cpu, wall};
			}
			m_depth++;
		}

		inline void Exit(Region r)
		{
			(void)r;
			if (m_depth == 0)
			{
				if (m_totals)
					m_totals->unbalanced_exits++;
				return;
			}
			m_depth--;
			if (m_depth >= kMaxDepth)
				return;
			if (m_active)
			{
				const std::uint64_t cpu = m_cpu_clock();
				const std::uint64_t wall = m_wall_clock();
				Charge(m_stack[m_depth], cpu, wall);
				if (m_totals)
					m_totals->region_samples[static_cast<std::size_t>(m_stack[m_depth].region)]++;
				if (m_depth > 0)
				{
					m_stack[m_depth - 1].cpu_start = cpu;
					m_stack[m_depth - 1].wall_start = wall;
				}
			}
			if (m_depth == 0)
				m_active = false;
		}

		bool IsActive() const { return m_active; }
		std::size_t GetDepth() const { return m_depth; }
		std::uint32_t GetCounter() const { return m_counter; }
		void SetTotals(Totals* totals) { m_totals = totals; }
		void Reset()
		{
			m_depth = 0;
			m_counter = 0;
			m_active = false;
		}

	private:
		inline void Charge(const StackEntry& e, std::uint64_t cpu, std::uint64_t wall)
		{
			if (!m_totals)
				return;
			const auto reg_idx = static_cast<std::size_t>(e.region);
			if (cpu >= e.cpu_start)
				m_totals->region_cpu_ns[reg_idx] += (cpu - e.cpu_start);
			else
				m_totals->clock_anomalies++;

			if (wall >= e.wall_start)
				m_totals->region_wall_ns[reg_idx] += (wall - e.wall_start);
			else
				m_totals->clock_anomalies++;
		}

		Totals* m_totals = nullptr;
		ClockFn m_cpu_clock = ReadCpuNs;
		ClockFn m_wall_clock = ReadWallNs;
		std::array<StackEntry, kMaxDepth> m_stack{};
		std::size_t m_depth = 0;
		std::uint32_t m_counter = 0;
		bool m_active = false;
	};

	inline std::uint32_t CompressedUsefulBytes(std::uint32_t width, std::uint32_t height, std::uint32_t block_dim, std::uint32_t bytes_per_block)
	{
		return ((width + block_dim - 1) / block_dim) * ((height + block_dim - 1) / block_dim) * bytes_per_block;
	}

#if GS_TEXTURE_COST_STATS
	extern Totals g_tc_totals;
	extern StackTimer g_tc_timer;

	class Scope
	{
	public:
		inline Scope(StackTimer& timer, Region region)
			: m_timer(timer)
			, m_region(region)
		{
			m_timer.Enter(m_region);
		}

		inline ~Scope()
		{
			m_timer.Exit(m_region);
		}

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

	private:
		StackTimer& m_timer;
		Region m_region;
	};
#endif
} // namespace GSTextureCost

#if GS_TEXTURE_COST_STATS
#define GS_TEXTURE_COST_SCOPE(region) GSTextureCost::Scope tc_scope_##region(GSTextureCost::g_tc_timer, GSTextureCost::Region::region)
#define GS_TEXTURE_COST_BEGIN(region) GSTextureCost::g_tc_timer.Enter(GSTextureCost::Region::region)
#define GS_TEXTURE_COST_END(region) GSTextureCost::g_tc_timer.Exit(GSTextureCost::Region::region)
#define GS_TEXTURE_COST_INC(counter) (GSTextureCost::g_tc_totals.counter++)
#define GS_TEXTURE_COST_ADD(counter, val) (GSTextureCost::g_tc_totals.counter += (val))
#else
#define GS_TEXTURE_COST_SCOPE(region)
#define GS_TEXTURE_COST_BEGIN(region)
#define GS_TEXTURE_COST_END(region)
#define GS_TEXTURE_COST_INC(counter)
#define GS_TEXTURE_COST_ADD(counter, val)
#endif
