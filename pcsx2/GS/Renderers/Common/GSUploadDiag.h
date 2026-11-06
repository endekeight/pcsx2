// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#ifndef GS_DIAG_UPLOAD_STATS
#define GS_DIAG_UPLOAD_STATS 0
#endif
#ifndef GS_DIAG_UPLOAD_CLIENT_MEM
#define GS_DIAG_UPLOAD_CLIENT_MEM 0
#endif
#ifndef GS_DIAG_UPLOAD_FRESH_DEST
#define GS_DIAG_UPLOAD_FRESH_DEST 0
#endif

static_assert(!(GS_DIAG_UPLOAD_CLIENT_MEM && GS_DIAG_UPLOAD_FRESH_DEST),
	"GS_DIAG_UPLOAD_CLIENT_MEM and GS_DIAG_UPLOAD_FRESH_DEST are mutually exclusive");
static_assert(!GS_DIAG_UPLOAD_FRESH_DEST || GS_DIAG_UPLOAD_STATS, "GS_DIAG_UPLOAD_FRESH_DEST needs GS_DIAG_UPLOAD_STATS (its budget resets with the stats window)");

#include <cstdint>
#include <cstdio>
#include <string>

namespace GSUploadDiag
{
	struct Counters
	{
		std::uint64_t uploads = 0;
		std::uint64_t bytes = 0;
		std::uint64_t uploads_compressed = 0;
		std::uint64_t uploads_client_mem = 0;
		std::uint64_t uploads_stream = 0;
		std::uint64_t uploads_into_busy_dest = 0;
		std::uint64_t fetches = 0;
		std::uint64_t fetch_reused = 0;
		std::uint64_t fetch_reused_busy = 0;
		std::uint64_t fetch_created = 0;
		std::uint64_t armb_eligible = 0;
		std::uint64_t armb_taken = 0;
		std::uint64_t armb_fresh = 0;
		std::uint64_t armb_busy_skips = 0;
		std::uint64_t armb_fallbacks = 0;
		std::uint64_t armb_fresh_bytes = 0;
	};

#if GS_DIAG_UPLOAD_STATS || GS_DIAG_UPLOAD_FRESH_DEST
	inline Counters g_counters;
#endif

	constexpr std::uint64_t kArmBFreshBudgetBytes = 32ull * 1024 * 1024;

	constexpr bool ArmBBudgetAllows(std::uint64_t fresh_bytes_so_far, std::uint64_t new_bytes)
	{
		return fresh_bytes_so_far <= kArmBFreshBudgetBytes &&
			   new_bytes <= kArmBFreshBudgetBytes - fresh_bytes_so_far;
	}

	constexpr bool ArmAEligible(bool compressed)
	{
		return !compressed;
	}

	inline std::string FormatLine(const Counters& c, long long ms, unsigned long long vsyncs)
	{
		char buf[1024];
		std::snprintf(buf, sizeof(buf),
			"GSUPLOAD: ms=%lld vsyncs=%llu uploads=%llu bytes=%llu uploads_compressed=%llu "
			"uploads_client_mem=%llu uploads_stream=%llu uploads_into_busy_dest=%llu fetches=%llu "
			"fetch_reused=%llu fetch_reused_busy=%llu fetch_created=%llu armb_eligible=%llu "
			"armb_taken=%llu armb_fresh=%llu armb_busy_skips=%llu armb_fallbacks=%llu armb_fresh_bytes=%llu",
			ms, vsyncs,
			static_cast<unsigned long long>(c.uploads),
			static_cast<unsigned long long>(c.bytes),
			static_cast<unsigned long long>(c.uploads_compressed),
			static_cast<unsigned long long>(c.uploads_client_mem),
			static_cast<unsigned long long>(c.uploads_stream),
			static_cast<unsigned long long>(c.uploads_into_busy_dest),
			static_cast<unsigned long long>(c.fetches),
			static_cast<unsigned long long>(c.fetch_reused),
			static_cast<unsigned long long>(c.fetch_reused_busy),
			static_cast<unsigned long long>(c.fetch_created),
			static_cast<unsigned long long>(c.armb_eligible),
			static_cast<unsigned long long>(c.armb_taken),
			static_cast<unsigned long long>(c.armb_fresh),
			static_cast<unsigned long long>(c.armb_busy_skips),
			static_cast<unsigned long long>(c.armb_fallbacks),
			static_cast<unsigned long long>(c.armb_fresh_bytes));
		return std::string(buf);
	}

	inline void Reset(Counters& c)
	{
		c = Counters{};
	}
} // namespace GSUploadDiag
