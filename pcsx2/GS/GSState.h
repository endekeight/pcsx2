// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/GS.h"
#include "GS/GSPerfMon.h"
#include "GS/GSLocalMemory.h"
#include "GS/GSDrawingContext.h"
#include "GS/GSDrawingEnvironment.h"
#include "GS/Renderers/Common/GSVertex.h"
#include "GS/Renderers/Common/GSVertexTrace.h"
#include "GS/Renderers/Common/GSDevice.h"
#include "GS/GSVector.h"
#include "GS/GSGifFused.h"
#include "GSAlignedClass.h"

// C110 diagnostic: counts GIF packed-vertex parsing and auto-flush work for the GSPARSE line. Ships 0.
// Defined only here, never on the compiler command line: GSState's layout depends on it,
// so every translation unit must see the same value.
#define GS_PARSE_STATS 0

// Count-only census of GIF packed register layouts, by primitive type. Ships 0.
// Defined only here, never on the compiler command line: GSState's layout depends on it,
// so every translation unit must see the same value.
#define GS_GIF_LAYOUT_STATS 0

#if GS_PARSE_STATS
#include <chrono>
#endif

#if GS_GIF_LAYOUT_STATS
#include <chrono>
#endif
#if GS_GIF_FUSED_VERIFY
#include <chrono>
#include <vector>
#endif

class GSDumpBase;

class GSState : public GSAlignedClass<32>
{
public:
	GSState();
	virtual ~GSState();

	static constexpr int GetSaveStateSize(int version);

private:
	// RESTRICT prevents multiple loads of the same part of the register when accessing its bitfields (the compiler is happy to know that memory writes in-between will not go there)

	typedef void (GSState::*GIFPackedRegHandler)(const GIFPackedReg* RESTRICT r);

	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16] = {};
	GIFPackedRegHandler m_fpGIFPackedRegHandlerXYZ[8][4] = {};

	void CheckFlushes();

	void GIFPackedRegHandlerNull(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerRGBA(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerSTQ(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerUV(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerUV_Hack(const GIFPackedReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush> void GIFPackedRegHandlerXYZF2(const GIFPackedReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush> void GIFPackedRegHandlerXYZ2(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerFOG(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerA_D(const GIFPackedReg* RESTRICT r);
	void GIFPackedRegHandlerNOP(const GIFPackedReg* RESTRICT r);

	typedef void (GSState::*GIFRegHandler)(const GIFReg* RESTRICT r);

	GIFRegHandler m_fpGIFRegHandlers[256] = {};
	GIFRegHandler m_fpGIFRegHandlerXYZ[8][4] = {};

	typedef void (GSState::*GIFPackedRegHandlerC)(const GIFPackedReg* RESTRICT r, u32 size);

	GIFPackedRegHandlerC m_fpGIFPackedRegHandlersC[2] = {};
	GIFPackedRegHandlerC m_fpGIFPackedRegHandlerSTQRGBAXYZF2[8] = {};
	GIFPackedRegHandlerC m_fpGIFPackedRegHandlerSTQRGBAXYZ2[8] = {};

	template<u32 prim, bool auto_flush> void GIFPackedRegHandlerSTQRGBAXYZF2(const GIFPackedReg* RESTRICT r, u32 size);
	template<u32 prim, bool auto_flush> void GIFPackedRegHandlerSTQRGBAXYZ2(const GIFPackedReg* RESTRICT r, u32 size);
	void GIFPackedRegHandlerNOP(const GIFPackedReg* RESTRICT r, u32 size);

#if GS_GIF_FUSED_LAYOUTS
	typedef void (GSState::*GIFPackedRegHandlerFusedL2)(const GIFPackedReg* RESTRICT r, u32 size, u32 pairs);
	GIFPackedRegHandlerFusedL2 m_fpGIFPackedRegHandlerRGBASTXYZF2[8] = {};
	template<u32 prim, bool auto_flush> void GIFPackedRegHandlerRGBASTXYZF2(const GIFPackedReg* RESTRICT r, u32 size, u32 pairs);
	typedef void (GSState::*GIFPackedRegHandlerFusedL1)(const GIFPackedReg* RESTRICT r, u32 size);
	GIFPackedRegHandlerFusedL1 m_fpGIFPackedRegHandlerADSTQRGBAXYZF2[8] = {};
	template<u32 prim, bool auto_flush> void GIFPackedRegHandlerADSTQRGBAXYZF2(const GIFPackedReg* RESTRICT r, u32 size);
	typedef void (GSState::*GIFPackedRegHandlerFusedL3)(const GIFPackedReg* RESTRICT r, u32 size);
	GIFPackedRegHandlerFusedL3 m_fpGIFPackedRegHandlerSTQRGBAXYZ2x4[8] = {};
	template<u32 prim, bool auto_flush> void GIFPackedRegHandlerSTQRGBAXYZ2x4(const GIFPackedReg* RESTRICT r, u32 size);
#endif

	template<int i> void ApplyTEX0(GIFRegTEX0& TEX0);
	void ApplyPRIM(u32 prim);

	void GIFRegHandlerNull(const GIFReg* RESTRICT r);
	void GIFRegHandlerPRIM(const GIFReg* RESTRICT r);
	void GIFRegHandlerRGBAQ(const GIFReg* RESTRICT r);
	void GIFRegHandlerST(const GIFReg* RESTRICT r);
	void GIFRegHandlerUV(const GIFReg* RESTRICT r);
	void GIFRegHandlerUV_Hack(const GIFReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush> void GIFRegHandlerXYZF2(const GIFReg* RESTRICT r);
	template<u32 prim, u32 adc, bool auto_flush> void GIFRegHandlerXYZ2(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEX0(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerCLAMP(const GIFReg* RESTRICT r);
	void GIFRegHandlerFOG(const GIFReg* RESTRICT r);
	void GIFRegHandlerNOP(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEX1(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEX2(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerXYOFFSET(const GIFReg* RESTRICT r);
	void GIFRegHandlerPRMODECONT(const GIFReg* RESTRICT r);
	void GIFRegHandlerPRMODE(const GIFReg* RESTRICT r);
	void GIFRegHandlerTEXCLUT(const GIFReg* RESTRICT r);
	void GIFRegHandlerSCANMSK(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerMIPTBP1(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerMIPTBP2(const GIFReg* RESTRICT r);
	void GIFRegHandlerTEXA(const GIFReg* RESTRICT r);
	void GIFRegHandlerFOGCOL(const GIFReg* RESTRICT r);
	void GIFRegHandlerTEXFLUSH(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerSCISSOR(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerALPHA(const GIFReg* RESTRICT r);
	void GIFRegHandlerDIMX(const GIFReg* RESTRICT r);
	void GIFRegHandlerDTHE(const GIFReg* RESTRICT r);
	void GIFRegHandlerCOLCLAMP(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerTEST(const GIFReg* RESTRICT r);
	void GIFRegHandlerPABE(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerFBA(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerFRAME(const GIFReg* RESTRICT r);
	template<int i> void GIFRegHandlerZBUF(const GIFReg* RESTRICT r);
	void GIFRegHandlerBITBLTBUF(const GIFReg* RESTRICT r);
	void GIFRegHandlerTRXPOS(const GIFReg* RESTRICT r);
	void GIFRegHandlerTRXREG(const GIFReg* RESTRICT r);
	void GIFRegHandlerTRXDIR(const GIFReg* RESTRICT r);
	void GIFRegHandlerHWREG(const GIFReg* RESTRICT r);

	template<bool auto_flush> void SetPrimHandlers();

	struct GSTransferBuffer
	{
		int x = 0, y = 0;
		int w = 0, h = 0;
		int start = 0, end = 0, total = 0;
		u8* buff = nullptr;
		GSVector4i rect = GSVector4i::zero();
		GIFRegBITBLTBUF m_blit = {};
		GIFRegTRXPOS m_pos = {};
		GIFRegTRXREG m_reg = {};
		bool write = false;

		GSTransferBuffer();
		~GSTransferBuffer();

		void Init(GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG, const GIFRegBITBLTBUF& blit, bool is_write);
		bool Update(int tw, int th, int bpp, int& len);

	} m_tr;

protected:
	static constexpr int INVALID_ALPHA_MINMAX = 500;
	static constexpr int MAX_DRAW_BUFFERS = 3;

#if GS_PARSE_STATS
	struct ParseStats
	{
		u64 kicks = 0;
		u64 af_kicks = 0;
		u64 af_checks = 0;
		u64 af_strip_skips = 0;
		u64 af_selfread = 0;
		u64 af_flushes = 0;
		u64 stq_calls = 0;
		u64 stq_af_calls = 0;
		u64 stq_verts = 0;
		u64 cf_calls = 0;
		u64 cf_flushes = 0;
		u64 vsyncs = 0;
		std::chrono::steady_clock::time_point last{};
	};

	ParseStats m_parse_stats{};
#endif

#if GS_GIF_LAYOUT_STATS
	struct GifLayoutEntry
	{
		u64 regs = 0;
		u8 nreg = 0;
		u8 type = 0;
		u8 prim = 0;
		u64 packets = 0;
		u64 qwords = 0;
	};

	struct GifLayoutStats
	{
		GifLayoutEntry table[64]{};
		u32 used = 0;
		u8 packed_prim[4]{};
		u64 overflow_packets = 0;
		u64 overflow_qwords = 0;
		u64 reglist_q = 0;
		u64 image_q = 0;
		u64 empty_tags = 0;
		u64 transfers = 0;
		std::chrono::steady_clock::time_point last{};
	};

	GifLayoutStats m_gif_layout_stats{};

	void AddGifLayoutPacked(const GIFPath& path, u8 prim, u64 packets, u64 qwords);
	void MaybePrintGifLayoutStats();
#endif

	GSVertex m_v = {};
	float m_q = 1.0f;
	GSVector4i m_xyof = {};
	int  m_used_buffers_idx = 0;
	int m_current_buffer_idx = 0;
	bool m_recent_buffer_switch = false;
	u32 m_max_vertex_count;

	struct GSVertexBuff
	{
		GSVertex* buff;
		GSVertex* buff_copy; // same size buffer to copy/modify the original buffer
		u32 head, tail, next; // head: first vertex, tail: last vertex + 1, next: last indexed + 1
		u32 xy_tail;
		GSVector4i xy[4];
		GSVector4i xyhead;
	};

	GSVertexBuff m_vertex_buffers[MAX_DRAW_BUFFERS];
	GSVertexBuff* m_vertex;

	struct GSIndexBuff
	{
		u16* buff;
		u32 tail;
	};

	GSIndexBuff m_index_buffers[MAX_DRAW_BUFFERS];

	GSIndexBuff* m_index;

	GSVertexBuff m_draw_vertex = {};

	struct
	{
		u16* buff;
		u32 tail;
	} m_draw_index = {};

	struct GSDrawBufferEnv
	{
		GSDrawingEnvironment m_env;
		int m_backed_up_ctx = 0;
		u32 m_dirty_regs = 0;
		GSVector4i draw_rect = GSVector4i::zero();
		bool related_draw = false;
	};

	GSDrawBufferEnv m_env_buffers[MAX_DRAW_BUFFERS] = {};

#if GS_GIF_FUSED_VERIFY
	struct FusedLayoutCounters
	{
		u64 seen = 0;
		u64 accepted = 0;
		u64 refused = 0;
		u64 verify_a = 0;
		u64 verify_ok = 0;
		u64 verify_f = 0;
		u64 verify_f_ok = 0;
		u64 skip_write = 0;
		u64 skip_grow = 0;
		u64 skip_move = 0;
		u64 skip_partial = 0;
		u64 skip_state = 0;
		u64 l1_break_prim = 0;
		u64 l1_refused_ad = 0;
	};

	struct FusedVerifySnap
	{
		GSVertex v = {};
		float q = 1.0f;
		u32 dirty_gs_regs = 0;
		int backed_up_ctx = 0;
		GSVector4i temp_draw_rect = GSVector4i::zero();
		GSDrawingEnvironment env = {};
		GSDrawingEnvironment prev_env = {};
		bool texflush_flag = false;
		bool isPackedUV_HackFlag = false;
		int tr_x = 0, tr_y = 0, tr_w = 0, tr_h = 0;
		int tr_start = 0, tr_end = 0, tr_total = 0;
		GSVector4i tr_rect = GSVector4i::zero();
		GIFRegBITBLTBUF tr_blit = {};
		GIFRegTRXPOS tr_pos = {};
		GIFRegTRXREG tr_reg = {};
		bool tr_write = false;
		int current_buffer_idx = 0;
		int used_buffers_idx = 0;
		bool recent_buffer_switch = false;
		GSVector4i xyof = {};
		bool scissor_invalid = false;
		GIFPackedRegHandler packed_xyz[8][4] = {};
		GIFRegHandler reg_xyz[8][4] = {};
		GIFPackedRegHandlerC packed_c[2] = {};
		GIFPackedRegHandler packed_handlers[16] = {};
		GIFRegHandler reg_handlers[256] = {};
#if GS_GIF_FUSED_LAYOUTS
		GIFPackedRegHandlerFusedL2 l2_handlers[8] = {};
		GIFPackedRegHandlerFusedL1 l1_handlers[8] = {};
		GIFPackedRegHandlerFusedL3 l3_handlers[8] = {};
#endif
		u32 kicks = 0;
		u32 flush_reason = 0;
		GSDrawBufferEnv env_buffers[MAX_DRAW_BUFFERS] = {};
		u32 v_head[MAX_DRAW_BUFFERS] = {};
		u32 v_tail[MAX_DRAW_BUFFERS] = {};
		u32 v_next[MAX_DRAW_BUFFERS] = {};
		u32 v_xy_tail[MAX_DRAW_BUFFERS] = {};
		GSVector4i v_xy[MAX_DRAW_BUFFERS][4] = {};
		GSVector4i v_xyhead[MAX_DRAW_BUFFERS] = {};
		u32 i_tail[MAX_DRAW_BUFFERS] = {};
		std::vector<GSVertex> vbuff[MAX_DRAW_BUFFERS];
		std::vector<u16> ibuff[MAX_DRAW_BUFFERS];
		GSClut::VerifyState clut = {};
		bool channel_shuffle_finish = false;
		bool trial_inval = false;
	};

	FusedLayoutCounters m_fused_counters[3]{};
	FusedVerifySnap m_fused_snap{};
	FusedVerifySnap m_fused_post{};
	FusedVerifySnap m_fused_flush{};
	std::chrono::steady_clock::time_point m_fused_stats_last{};
	int m_fused_verify_guard = 0;
	int m_fused_verify_trial = 0;
	u32 m_fused_verify_skip = 0;
	u32 m_fused_trial_kicks = 0;
	bool m_fused_flush_captured = false;
	bool m_fused_trial_inval = false;
	const u8** m_fused_verify_mem = nullptr;
	u32 m_fused_verify_total = 0;
	u64 m_fused_l1_addr[128]{};

	void FusedVerifyCapture(FusedVerifySnap& s);
	bool FusedVerifyRestore(const FusedVerifySnap& s);
	bool FusedVerifyCompare(const FusedVerifySnap& a, const FusedVerifySnap& b, const char* layout, u64 packet);
	void FusedVerifyFail(const char* layout, const char* field, u64 packet, const char* values);
	void MaybePrintFusedStats();
	void FusedVerifyNoteWrite()
	{
		if (m_fused_verify_guard)
			m_fused_verify_skip |= 2u;
	}
	void FusedVerifyNoteGrow()
	{
		if (m_fused_verify_guard)
			m_fused_verify_skip |= 4u;
	}
	void FusedVerifyNoteMove()
	{
		if (m_fused_verify_guard)
			m_fused_verify_skip |= 8u;
	}
#if GS_GIF_FUSED_LAYOUTS
	void FusedVerifyPerQuadword(GIFPath& path, const u8* mem, u32 total);
	bool FusedVerifyTakeSkip(FusedLayoutCounters& c);
	void FusedVerifyMutate(FusedVerifySnap& s, int layout);
	void FusedVerifyDualRun(GIFPath& path, const u8*& mem, u32 total, int layout, const char* name);
	void FusedVerifyRunL1(GIFPath& path, const u8*& mem, u32 total, GIFPackedRegHandlerFusedL1 handler);
	void FusedVerifyRunL2(GIFPath& path, const u8*& mem, u32 total, u32 pairs, GIFPackedRegHandlerFusedL2 handler);
	void FusedVerifyRunL3(GIFPath& path, const u8*& mem, u32 total, GIFPackedRegHandlerFusedL3 handler);
	GIFPackedRegHandlerFusedL1 m_fused_run_l1 = nullptr;
	GIFPackedRegHandlerFusedL2 m_fused_run_l2 = nullptr;
	GIFPackedRegHandlerFusedL3 m_fused_run_l3 = nullptr;
	u32 m_fused_run_pairs = 0;
#endif
#endif

	void UpdateContext();
	void UpdateScissor();

	void UpdateVertexKick();

	void GrowVertexBuffer();
	bool IsAutoFlushDraw(u32 prim, int& tex_layer);
	template<u32 prim> void HandleAutoFlush();
	bool EarlyDetectShuffle(u32 prim);
	void CheckCLUTValidity(u32 prim);
	bool CheckOverlapVerts(u32 n);

	template <u32 prim, bool auto_flush> void VertexKick(u32 skip);

	// following functions need m_vt to be initialized

	GSVertexTrace m_vt;
	GSVertexTrace::VertexAlpha& GetAlphaMinMax()
	{
		if (!m_vt.m_alpha.valid)
			CalcAlphaMinMax(0, INVALID_ALPHA_MINMAX);
		return m_vt.m_alpha;
	}
	struct TextureMinMaxResult
	{
		enum UsesBoundary
		{
			USES_BOUNDARY_LEFT   = 1 << 0,
			USES_BOUNDARY_TOP    = 1 << 1,
			USES_BOUNDARY_RIGHT  = 1 << 2,
			USES_BOUNDARY_BOTTOM = 1 << 3,
			USES_BOUNDARY_U = USES_BOUNDARY_LEFT | USES_BOUNDARY_RIGHT,
			USES_BOUNDARY_V = USES_BOUNDARY_TOP | USES_BOUNDARY_BOTTOM,
		};
		GSVector4i coverage; ///< Part of the texture used
		u8 uses_boundary;    ///< Whether or not the usage touches the left, top, right, or bottom edge (and therefore needs wrap modes preserved)
	};
	TextureMinMaxResult GetTextureMinMax(GIFRegTEX0 TEX0, GIFRegCLAMP CLAMP, bool linear, bool clamp_to_tsize);
	bool TryAlphaTest(u32& fm, u32& zm);
	bool IsFlatShaded();
	bool IsOpaque();
	bool IsMipMapDraw();
	bool IsMipMapActive();
	bool IsCoverageAlpha();
	bool IsCoverageAlphaFixedOne();
	virtual bool IsCoverageAlphaSupported();
	void CalcAlphaMinMax(const int tex_min, const int tex_max);
	void CorrectATEAlphaMinMax(const u32 atst, const int aref);

	// Utility functions for getting position/texture coordinates.
	GSVector4 GetXYWindow(const GSVertex& v);
	template<bool fst>
	GSVector4 GetTexCoordsImpl(const GSVertex& v, float q);
	template<bool fst>
	GSVector4 GetTexCoordsImpl(const GSVertex& v);
	GSVector4 GetTexCoords(const GSVertex& v, float q);
	GSVector4 GetTexCoords(const GSVertex& v);

	// Utility functions to detect and get corners of quads.
	template<u32 primclass, bool tme = false, bool fst = false>
	static bool GetQuadCornersImpl(const GSVertex* v, const u16* i, GSVertex& vout0, GSVertex& vout1);
	bool GetQuadCorners(const GSVertex* v, const u16* i, GSVertex& vout0, GSVertex& vout1);

	// Utility functions to get window/texture coordinates of a quad.
	template<u32 primclass>
	void GetQuadBBoxWindowImpl(const GSVertex& v0, const GSVertex& v1, GSVector4& xyout);
	template<u32 primclass, bool tme = false, bool fst = false>
	void GetQuadBBoxWindowImpl(const GSVertex& v0, const GSVertex& v1, GSVector4& xyout, GSVector4& texout, bool keep_tex_order = true);
	void GetQuadBBoxWindow(const GSVertex& v0, const GSVertex& v1, GSVector4& xyout);
	void GetQuadBBoxWindow(const GSVertex& v0, const GSVertex& v1, GSVector4& xyout, GSVector4& texout, bool keep_tex_order = true);

	// Adjusts a quad so that it contains exactly the centers of the pixels that the GS would rasterize.
	static void GetQuadRasterizedPoints(GSVector4& xy, bool keep_order = true);
	static void GetQuadRasterizedPoints(GSVector4& xy, GSVector4& tex, bool keep_order = true);

public:
#if GS_GIF_FUSED_VERIFY
	void FusedVerifyNoteInval()
	{
		if (m_fused_verify_trial)
			m_fused_trial_inval = true;
	}
	bool FusedVerifyIsTrial() const { return m_fused_verify_trial != 0; }
#endif
	enum EEGS_TransferType
	{
		EE_to_GS,
		GS_to_GS,
		GS_to_EE,
		Clear
	};

	struct GSUploadQueue
	{
		GIFRegBITBLTBUF blit;
		u64 draw;
		GSVector4i rect;
		EEGS_TransferType transfer_type;
	};

	enum NoGapsType
	{
		Uninitialized = 0,
		GapsFound,
		SpriteNoGaps,
		FullCover,
	};

	GIFPath m_path[4] = {};
	const GIFRegPRIM* PRIM = nullptr;
	GSPrivRegSet* m_regs = nullptr;
	GSLocalMemory m_mem;
	GSDrawingEnvironment m_env = {};
	GSDrawingEnvironment m_prev_env = {};
	GSDrawingEnvironment m_temp_env = {};
	const GSDrawingEnvironment* m_draw_env = &m_env;
	GSDrawingContext* m_context = nullptr;
	GSVector4i temp_draw_rect;
	std::unique_ptr<GSDumpBase> m_dump;
	bool m_scissor_invalid = false;
	bool m_quad_check_valid = false;
	bool m_quad_check_valid_shuffle = false;
	bool m_are_quads = false;
	bool m_are_quads_shuffle = false;
	bool m_nativeres = false;
	bool m_mipmap = false;
	bool m_texflush_flag = false;
	bool m_isPackedUV_HackFlag = false;
	bool m_channel_shuffle = false;
	bool m_using_temp_z = false;
	bool m_temp_z_full_copy = false;
	bool m_in_target_draw = false;
	bool m_channel_shuffle_finish = false;

	u32 m_target_offset = 0;
	u8 m_scanmask_used = 0;
	u32 m_dirty_gs_regs = 0;
	int m_backed_up_ctx = 0;
	std::vector<GSUploadQueue> m_draw_transfers;
	NoGapsType m_primitive_covers_without_gaps;
	GSVector4i m_r = {};
	GSVector4i m_r_no_scissor = {};

	static u64 s_n;
	static u64 s_last_transfer_draw_n;
	static u64 s_transfer_n;

	GSPerfMon m_perfmon_frame; // Track stat across a frame.
	GSPerfMon m_perfmon_draw;  // Track stat across a draw.

	static constexpr u32 STATE_VERSION = 9;

	#define PRIM_REG_MASK 0x7FF
	#define MIPTBP_REG_MASK ((1ULL << 60) - 1ULL)
	#define CLAMP_REG_MASK ((1ULL << 44) - 1ULL)
	#define TEX1_REG_MASK 0xFFF001803FDULL
	#define XYOFFSET_REG_MASK 0x0000FFFF0000FFFFULL
	#define TEXA_REG_MASK 0xFF000080FFULL
	#define FOGCOL_REG_MASK 0xFFFFFF
	#define SCISSOR_REG_MASK 0x7FF07FF07FF07FFULL
	#define ALPHA_REG_MASK 0xFF000000FFULL
	#define DIMX_REG_MASK 0x7777777777777777ULL
	#define FRAME_REG_MASK 0xFFFFFFFF3F3F01FFULL
	#define ZBUF_REG_MASK 0x10F0001FFULL
	#define TEST_REG_MASK 0x7FFFF

	enum REG_DIRTY
	{
		DIRTY_REG_ALPHA,
		DIRTY_REG_CLAMP,
		DIRTY_REG_COLCLAMP,
		DIRTY_REG_DIMX,
		DIRTY_REG_DTHE,
		DIRTY_REG_FBA,
		DIRTY_REG_FOGCOL,
		DIRTY_REG_FRAME,
		DIRTY_REG_MIPTBP1,
		DIRTY_REG_MIPTBP2,
		DIRTY_REG_PABE,
		DIRTY_REG_PRIM,
		DIRTY_REG_SCANMSK,
		DIRTY_REG_SCISSOR,
		DIRTY_REG_TEST,
		DIRTY_REG_TEX0,
		DIRTY_REG_TEX1,
		DIRTY_REG_TEXA,
		DIRTY_REG_XYOFFSET,
		DIRTY_REG_ZBUF
	};

	enum GSFlushReason
	{
		UNKNOWN = 1 << 0,
		RESET = 1 << 1,
		CONTEXTCHANGE = 1 << 2,
		CLUTCHANGE = 1 << 3,
		GSTRANSFER = 1 << 4,
		UPLOADDIRTYTEX = 1 << 5,
		UPLOADDIRTYFRAME = 1 << 6,
		UPLOADDIRTYZBUF = 1 << 7,
		LOCALTOLOCALMOVE = 1 << 8,
		DOWNLOADFIFO = 1 << 9,
		SAVESTATE = 1 << 10,
		LOADSTATE = 1 << 11,
		AUTOFLUSH = 1 << 12,
		VSYNC  = 1 << 13,
		GSREOPEN = 1 << 14,
		VERTEXCOUNT = 1 << 15,
	};

	GSFlushReason m_state_flush_reason = UNKNOWN;

	enum PRIM_OVERLAP
	{
		PRIM_OVERLAP_UNKNOW,
		PRIM_OVERLAP_YES,
		PRIM_OVERLAP_NO
	};

	PRIM_OVERLAP m_prim_overlap = PRIM_OVERLAP_UNKNOW;
	std::vector<size_t> m_drawlist;
	std::vector<GSVector4i> m_drawlist_bbox;

	struct GSPCRTCRegs
	{
		struct PCRTCDisplay
		{
			bool enabled;
			int FBP;
			int FBW;
			int PSM;
			int DBY;
			int DBX;
			GSRegDISPFB prevFramebufferReg;
			GSVector2i prevDisplayOffset;
			GSVector2i displayOffset;
			GSVector4i displayRect;
			GSVector2i magnification;
			GSVector2i prevFramebufferOffsets;
			GSVector2i framebufferOffsets;
			GSVector4i framebufferRect;

			__fi int Block() const { return FBP << 5; }
		};

		int videomode = 0;
		int interlaced = 0;
		int FFMD = 0;
		bool PCRTCSameSrc = false;
		bool toggling_field = false;
		PCRTCDisplay PCRTCDisplays[2] = {};

		bool IsAnalogue();

		// Calculates which display is closest to matching zero offsets in either direction.
		GSVector2i NearestToZeroOffset();

		void SetVideoMode(GSVideoMode videoModeIn);

		// Enable each of the displays.
		void EnableDisplays(GSRegPMODE pmode, GSRegSMODE2 smode2, bool smodetoggle);

		void CheckSameSource();
		
		bool FrameWrap();

		// If the start point of both frames match, we can do a single read
		bool FrameRectMatch();

		GSVector2i GetResolution();

		GSVector4i GetFramebufferRect(int display);

		int GetFramebufferBitDepth();

		GSVector2i GetFramebufferSize(int display);

		// Sets up the rectangles for both the framebuffer read and the displays for the merge circuit.
		void SetRects(int display, GSRegDISPLAY displayReg, GSRegDISPFB framebufferReg);

		// Calculate framebuffer read offsets, should be considered if only one circuit is enabled, or difference is more than 1 line.
		// Only considered if "Anti-blur" is enabled.
		void CalculateFramebufferOffset(bool scanmask, GSRegDISPFB framebuffer0Reg, GSRegDISPFB framebuffer1Reg);

		// Used in software mode to align the buffer when reading. Offset is accounted for (block aligned) by GetOutput.
		void RemoveFramebufferOffset(int display);

		// If the two displays are offset from each other, move them to the correct offsets.
		// If using screen offsets, calculate the positions here.
		void CalculateDisplayOffset(bool scanmask);
	} PCRTCDisplays;

public:
	/// Returns the appropriate directory for draw dumping.
	static std::string GetDrawDumpPath(const char* format, ...);

	/// Expands dither matrix, suitable for software renderer.
	static void ExpandDIMX(GSVector4i* dimx, const GIFRegDIMX DIMX);

	/// Returns a string representing the flush reason.
	static const char* GetFlushReasonString(GSFlushReason reason);

	void ResetHandlers();
	void ResetPCRTC();

	GSVideoMode GetVideoMode();

	bool isinterlaced();
	bool isReallyInterlaced();

	float GetTvRefreshRate();

	virtual void Reset(bool hardware_reset);
	virtual void UpdateSettings(const Pcsx2Config::GSOptions& old_config);

	void ResetDrawBuffers();
	void ResetDrawBufferIdx();
	void FlushBuffers(bool flush_base_only = false, bool use_flush_reason = false, GSFlushReason flush_reason = GSFlushReason::CONTEXTCHANGE);
	void PushBuffer();
	void SetDrawBufferEnv();
	void SetDrawBuffDirty();
	bool CanBufferNewDraw();
	void Flush(GSFlushReason reason);
	void FlushDraw(GSFlushReason reason);
	u32 CalcMask(int exp, int max_exp);
	void FlushPrim();
	bool TestDrawChanged();
	void FlushWrite();
	virtual void Draw() = 0;
	virtual void PurgeTextureCache(bool sources, bool targets, bool hash_cache);
	virtual void ReadbackTextureCache();
	virtual void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r) {}
	virtual void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false) {}

	virtual void Move();

	GSVector4i GetTEX0Rect(const GSDrawingContext& prev_ctx);
	void CheckWriteOverlap(bool req_write, bool req_read);
	void Write(const u8* mem, int len);
	void Read(u8* mem, int len);
	void InitReadFIFO(u8* mem, int len);

	void SoftReset(u32 mask);
	void WriteCSR(u32 csr) { m_regs->CSR.U32[1] = csr; }
	void ReadFIFO(u8* mem, int size);
	void ReadLocalMemoryUnsync(u8* mem, int qwc, GIFRegBITBLTBUF BITBLTBUF, GIFRegTRXPOS TRXPOS, GIFRegTRXREG TRXREG);
	template<int index> void Transfer(const u8* mem, u32 size);
	int Freeze(freezeData* fd, bool sizeonly);
	int Defrost(const freezeData* fd);

	u8* GetRegsMem() const { return reinterpret_cast<u8*>(m_regs); }
	void SetRegsMem(u8* basemem) { m_regs = reinterpret_cast<GSPrivRegSet*>(basemem); }

	void DumpDrawInfo(bool dump_regs, bool dump_verts, bool dump_transfers);
	void DumpVertices(const std::string& filename);
	void DumpTransferList(const std::string& filename);
	void DumpTransferImages();
	
	template<bool shuffle_check>
	bool TrianglesAreQuadsImpl();
	bool TrianglesAreQuads(bool shuffle_check = false);
	template <u32 primclass>
	PRIM_OVERLAP GetPrimitiveOverlapDrawlistImpl(bool save_drawlist = false, bool save_bbox = false,
		float bbox_scale = 1.0f, u32* max_size = nullptr);
	PRIM_OVERLAP GetPrimitiveOverlapDrawlist(bool save_drawlist = false, bool save_bbox = false,
		float bbox_scale = 1.0f, u32* max_size = nullptr);
	PRIM_OVERLAP PrimitiveOverlap(bool save_drawlist = false);
	bool SpriteDrawWithoutGaps();
	void CalculatePrimitiveCoversWithoutGaps();
	GIFRegTEX0 GetTex0Layer(u32 lod);
};

// We put this in the header because of Multi-ISA.
inline void GSState::ExpandDIMX(GSVector4i* dimx, const GIFRegDIMX DIMX)
{
	dimx[1] = GSVector4i(DIMX.DM00, 0, DIMX.DM01, 0, DIMX.DM02, 0, DIMX.DM03, 0);
	dimx[0] = dimx[1].xxzzlh();
	dimx[3] = GSVector4i(DIMX.DM10, 0, DIMX.DM11, 0, DIMX.DM12, 0, DIMX.DM13, 0);
	dimx[2] = dimx[3].xxzzlh();
	dimx[5] = GSVector4i(DIMX.DM20, 0, DIMX.DM21, 0, DIMX.DM22, 0, DIMX.DM23, 0);
	dimx[4] = dimx[5].xxzzlh();
	dimx[7] = GSVector4i(DIMX.DM30, 0, DIMX.DM31, 0, DIMX.DM32, 0, DIMX.DM33, 0);
	dimx[6] = dimx[7].xxzzlh();
}
