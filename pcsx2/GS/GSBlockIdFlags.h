// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#ifndef GS_BLOCK_ID_FLAGS_H
#define GS_BLOCK_ID_FLAGS_H

// Cheaper in-run identity of 256-byte local-memory blocks.
#ifndef GS_TC_BLOCK_ID
#define GS_TC_BLOCK_ID 0
#endif
// Oracle that the block-identity mixer matches the present hash-cache keys.
#ifndef GS_TC_BLOCK_ID_VERIFY
#define GS_TC_BLOCK_ID_VERIFY 0
#endif
// Select one mutation so the verify check must fire.
#ifndef GS_TC_BLOCK_ID_MUTATE
#define GS_TC_BLOCK_ID_MUTATE 0
#endif

#if GS_TC_BLOCK_ID_VERIFY && !GS_TC_BLOCK_ID
#error GS_TC_BLOCK_ID_VERIFY requires GS_TC_BLOCK_ID
#endif
#if GS_TC_BLOCK_ID_MUTATE && !GS_TC_BLOCK_ID_VERIFY
#error GS_TC_BLOCK_ID_MUTATE requires GS_TC_BLOCK_ID_VERIFY
#endif
#if GS_TC_BLOCK_ID_VERIFY && defined(NDEBUG)
#error GS_TC_BLOCK_ID_VERIFY is not for a build with NDEBUG
#endif
#if GS_TC_BLOCK_ID_MUTATE && defined(NDEBUG)
#error GS_TC_BLOCK_ID_MUTATE is not for a build with NDEBUG
#endif

#endif
