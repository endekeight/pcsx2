// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "vtlbUnmappedPhy.h"

#include "common/Console.h"

#include <mutex>

// Reached when a physical page has no handler. That should be rare once the memory map
// is set up, but some titles probe unmapped holes. Continuing is correct: an upstream
// Release build compiles pxFail out of these functions (common/Assertions.h), so they
// already return 0 and discard writes. Match that in every configuration so a Debug or
// Devel build does not turn a harmless probe into an abort. Each distinct address is
// logged once; a bounded table stops a guest loop from flooding the log or growing memory.

namespace
{
	constexpr int kMaxAddresses = 64;
	u32 s_seen[kMaxAddresses] = {};
	int s_n_seen = 0;
	std::mutex s_mutex;
} // namespace

void vtlbResetUnmappedPhysicalLog()
{
	// Clear so a second boot in this process logs the same probes again. The table is
	// otherwise process-lifetime, and Android keeps the process across title loads.
	std::lock_guard<std::mutex> guard(s_mutex);
	s_n_seen = 0;
}

static void vtlbReportUnmappedPhysical(const char* width, u32 addr)
{
	// Seen-address table: one warning per distinct address, capped at 64 so a
	// guest loop cannot flood the log or grow memory. EE, IOP and VU threads
	// all reach this slow path; the mutex keeps the table race-free. Mapped
	// page hits never enter here. Record under the lock, then log after release
	// — Console.Warning takes the log-file mutex (common/Console.cpp).
	{
		std::lock_guard<std::mutex> guard(s_mutex);
		for (int i = 0; i < s_n_seen; i++)
		{
			if (s_seen[i] == addr)
				return;
		}
		if (s_n_seen >= kMaxAddresses)
			return;
		s_seen[s_n_seen++] = addr;
	}
	Console.Warning("(VTLB) Ignored unmapped physical %s @ 0x%08X.", width, addr);
}

mem8_t vtlbDefaultPhyRead8(u32 addr)
{
	vtlbReportUnmappedPhysical("read8", addr);
	return 0;
}

mem16_t vtlbDefaultPhyRead16(u32 addr)
{
	vtlbReportUnmappedPhysical("read16", addr);
	return 0;
}

mem32_t vtlbDefaultPhyRead32(u32 addr)
{
	vtlbReportUnmappedPhysical("read32", addr);
	return 0;
}

mem64_t vtlbDefaultPhyRead64(u32 addr)
{
	vtlbReportUnmappedPhysical("read64", addr);
	return 0;
}

RETURNS_R128 vtlbDefaultPhyRead128(u32 addr)
{
	vtlbReportUnmappedPhysical("read128", addr);
	return r128_zero();
}

void vtlbDefaultPhyWrite8(u32 addr, mem8_t data)
{
	(void)data;
	vtlbReportUnmappedPhysical("write8", addr);
}

void vtlbDefaultPhyWrite16(u32 addr, mem16_t data)
{
	(void)data;
	vtlbReportUnmappedPhysical("write16", addr);
}

void vtlbDefaultPhyWrite32(u32 addr, mem32_t data)
{
	(void)data;
	vtlbReportUnmappedPhysical("write32", addr);
}

void vtlbDefaultPhyWrite64(u32 addr, mem64_t data)
{
	(void)data;
	vtlbReportUnmappedPhysical("write64", addr);
}

void TAKES_R128 vtlbDefaultPhyWrite128(u32 addr, r128 data)
{
	(void)data;
	vtlbReportUnmappedPhysical("write128", addr);
}
