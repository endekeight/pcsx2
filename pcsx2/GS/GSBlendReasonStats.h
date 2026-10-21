// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace GSBlendReason
{
	enum Reason : std::uint32_t
	{
		FetchForce,
		Fallback,
		OtherForce,
		BarrierReq,
		PreferSw,
		FreeBlend,
		BlendMix,
		NonRecursive,
		FreeColclip,
		AccumColclip,
		Other,
		Count,
		None = Count
	};
	inline constexpr std::array<const char*, Count> kNames = {
		"FetchForce", "Fallback", "OtherForce", "BarrierReq", "PreferSw", "FreeBlend",
		"BlendMix", "NonRecursive", "FreeColclip", "AccumColclip", "Other"};

	struct Inputs
	{
		bool fetch_force = false; // features.framebuffer_fetch && (one_barrier || require_full_barrier)
		bool fallback = false; // GS_DUAL_SOURCE_FALLBACK clause of force_sw_blending
		bool other_force = false; // (IsFeedbackLoopDepth && !depth_feedback) || UseDebugBlend
		bool barrier_req = false; // blend_requires_barrier, only when the Basic case ran
		bool prefer_sw = false; // prefer_sw_blend, only when the Basic case ran
		bool free_blend = false; // free_blend, only when the Basic case ran
		bool blend_mix = false; // blend_mix as or-ed into sw_blending in the Basic case
		bool non_recursive = false; // blend_non_recursive
		bool free_colclip = false; // colour-clip free branch taken
		bool accum_colclip = false; // colour-clip accumulation branch taken
	};

	// First true input in the order of the Reason enum; Other when none is true.
	constexpr Reason Select(const Inputs& in)
	{
		if (in.fetch_force)
			return FetchForce;
		if (in.fallback)
			return Fallback;
		if (in.other_force)
			return OtherForce;
		if (in.barrier_req)
			return BarrierReq;
		if (in.prefer_sw)
			return PreferSw;
		if (in.free_blend)
			return FreeBlend;
		if (in.blend_mix)
			return BlendMix;
		if (in.non_recursive)
			return NonRecursive;
		if (in.free_colclip)
			return FreeColclip;
		if (in.accum_colclip)
			return AccumColclip;
		return Other;
	}

	// Differs from PSSelector::IsFeedbackLoopRT() because that one ignores PS_PRIMID_INIT.
	// Mirrors NEEDS_RT in bin/resources/shaders/opengl/tfx_fs.glsl:56-66 term by term.
	constexpr bool NeedsRt(std::uint32_t tex_is_fb, std::uint32_t date, std::uint32_t afail,
		std::uint32_t fbmask, std::uint32_t blend_a, std::uint32_t blend_b, std::uint32_t blend_c,
		std::uint32_t blend_d, std::uint32_t a_masked)
	{
		const bool sw_blend = blend_a != 0 || blend_b != 0 || blend_d != 0;
		const bool sw_blend_needs_rt = sw_blend && (blend_a == 1 || blend_b == 1 || blend_c == 1 || blend_d == 1);
		const bool sw_ad_to_hw = blend_c == 1 && a_masked != 0;
		const bool primid_init = date == 1 || date == 2;
		const bool needs_rt_early = tex_is_fb == 1 || date >= 5;
		const bool needs_rt_afail = afail == 2 || afail == 3 || afail == 5;
		return needs_rt_early || needs_rt_afail || (!primid_init && (fbmask != 0 || sw_blend_needs_rt || sw_ad_to_hw));
	}

	// (z - x) * (w - y) * scale * scale as double; 0.0 when z <= x or w <= y.
	constexpr double RectArea(int x, int y, int z, int w, float scale)
	{
		if (z <= x || w <= y)
			return 0.0;
		return static_cast<double>(z - x) * static_cast<double>(w - y) * static_cast<double>(scale) * static_cast<double>(scale);
	}
} // namespace GSBlendReason
