// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "common/Pcsx2Defs.h"

#include <ctime>
#include <string>

// A35 diagnostic: counts per-present renderer work for the GS_FRAME_COST_STATS line. Ships 0.
// Defined only here, never on the compiler command line: GSPerfMon's layout depends on it,
// so every translation unit must see the same value.
#define GS_FRAME_COST_STATS 0

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
		SbReasonFetchForce, // B29: shader-blend draws whose first reason is the framebuffer-fetch force term.
		SbReasonFallback, // B29: shader-blend draws whose first reason is the dual-source fallback clause.
		SbReasonOtherForce, // B29: shader-blend draws whose first reason is depth feedback or debug blend force.
		SbReasonBarrierReq, // B29: shader-blend draws whose first reason is blend_requires_barrier under Basic.
		SbReasonPreferSw, // B29: shader-blend draws whose first reason is prefer_sw_blend under Basic.
		SbReasonFreeBlend, // B29: shader-blend draws whose first reason is free_blend under Basic.
		SbReasonBlendMix, // B29: shader-blend draws whose first reason is blend_mix under Basic.
		SbReasonNonRecursive, // B29: shader-blend draws whose first reason is blend_non_recursive under Minimum.
		SbReasonFreeColclip, // B29: shader-blend draws whose first reason is colour-clip free mode.
		SbReasonAccumColclip, // B29: shader-blend draws whose first reason is colour-clip accumulation mode.
		SbReasonOther, // B29: shader-blend draws with other or unmatched reason.
		NeedsRtFetchForce, // B29: NEEDS_RT draws whose shader-blend reason is fetch force.
		NeedsRtFallback, // B29: NEEDS_RT draws whose shader-blend reason is dual-source fallback.
		NeedsRtOtherForce, // B29: NEEDS_RT draws whose shader-blend reason is other force.
		NeedsRtBarrierReq, // B29: NEEDS_RT draws whose shader-blend reason is barrier required.
		NeedsRtPreferSw, // B29: NEEDS_RT draws whose shader-blend reason is prefer SW blend.
		NeedsRtFreeBlend, // B29: NEEDS_RT draws whose shader-blend reason is free blend.
		NeedsRtBlendMix, // B29: NEEDS_RT draws whose shader-blend reason is blend mix.
		NeedsRtNonRecursive, // B29: NEEDS_RT draws whose shader-blend reason is non-recursive blend.
		NeedsRtFreeColclip, // B29: NEEDS_RT draws whose shader-blend reason is free colour-clip.
		NeedsRtAccumColclip, // B29: NEEDS_RT draws whose shader-blend reason is accumulation colour-clip.
		NeedsRtOther, // B29: NEEDS_RT draws whose shader-blend reason is other.
		NeedsRtSb, // B29: shader-blend draws whose final pixel shader reads the framebuffer.
		NeedsRtAll, // B29: all hardware draws whose final pixel shader reads the framebuffer.
		ShaderBlendFinal, // B29: shader-blend draws that reached the end of setup without an ABORT_DRAW.
		AreaAll, // B29: bounding-rectangle pixels at render-target scale across all hardware draws.
		AreaSb, // B29: bounding-rectangle pixels at render-target scale across shader-blend draws.
		AreaNeedsRt, // B29: bounding-rectangle pixels at render-target scale across NEEDS_RT draws.
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
