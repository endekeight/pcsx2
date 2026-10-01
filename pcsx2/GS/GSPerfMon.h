// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "common/Pcsx2Defs.h"

#include <ctime>
#include <string>

// A35 diagnostic: counts per-present renderer work for the GS_FRAME_COST_STATS line. Ships 0.
#ifndef GS_FRAME_COST_STATS
#define GS_FRAME_COST_STATS 0
#endif

class GSPerfMon
{
public:
	enum counter_t
	{
		Prim,
		Draw,
		DrawCalls,
		Readbacks,
		Swizzle,
		Unswizzle,
		Fillrate,
		SyncPoint,
		Barriers,
		RenderPasses,
		TextureCopiesROV, // Overlaps with regular texture copies.
		DrawCallsROV, // Overlaps with regular draw calls.
		BarriersROV, // Overlaps with regular barriers.
#if GS_FRAME_COST_STATS
		ShaderBlendDraws, // Hardware draws that end with shader blending.
		DualSourceFallbackDraws, // Hardware draws shader-blended only because of the GS_DUAL_SOURCE_FALLBACK clause.
#endif
		CounterLast,

		// Reused counters for HW.
		TextureCopies = Fillrate,
		TextureUploads = SyncPoint,

		CounterLastHW = CounterLast,
		CounterLastSW = SyncPoint + 1
	};

protected:
	double m_counters[CounterLast] = {};
	double m_stats[CounterLast] = {};
#if GS_FRAME_COST_STATS
	// Running totals that Update() never clears.
	double m_totals[CounterLast] = {};
#endif
	int m_frame = 0;
	clock_t m_lastframe = 0;
	int m_count = 0;
	int m_disp_fb_sprite_blits = 0;

public:
	GSPerfMon();

	void Reset();

	void SetFrame(int frame) { m_frame = frame; }
	int GetFrame() { return m_frame; }
	void EndFrame(bool frame_only);

#if GS_FRAME_COST_STATS
	void Put(counter_t c, double val)
	{
		m_counters[c] += val;
		m_totals[c] += val;
	}
	double GetTotal(counter_t c) const { return m_totals[c]; }
#else
	void Put(counter_t c, double val) { m_counters[c] += val; }
#endif
	double GetCounter(counter_t c) { return m_counters[c]; }
	double Get(counter_t c) { return m_stats[c]; }
	void Update();

	__fi void AddDisplayFramebufferSpriteBlit() { m_disp_fb_sprite_blits++; }
	__fi int GetDisplayFramebufferSpriteBlits()
	{
		const int blits = m_disp_fb_sprite_blits;
		m_disp_fb_sprite_blits = 0;
		return blits;
	}

	GSPerfMon operator-(const GSPerfMon& other);

	void Dump(const std::string& filename, bool hw);
};

extern GSPerfMon g_perfmon;
