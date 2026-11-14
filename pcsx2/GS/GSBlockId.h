// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/GSRegs.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace GSBlockId
{
	// Starting value for a Combine loop over block identities.
	inline constexpr u64 kCombineInit = 0x9E3779B97F4A7C15ULL;

	inline u64 Mix64(u64 x)
	{
		x ^= x >> 33;
		x *= 0xff51afd7ed558ccdULL;
		x ^= x >> 33;
		x *= 0xc4ceb9fe1a85ec53ULL;
		x ^= x >> 33;
		return x;
	}

	// Identity of exactly GS_BLOCK_SIZE (256) bytes. `block` must be 16-byte aligned.
	u64 Hash(const u8* block);

	// Order-sensitive fold of one more identity into an accumulator.
	inline u64 Combine(u64 acc, u64 id)
	{
		return Mix64(acc ^ Mix64(id + 0x9E3779B97F4A7C15ULL));
	}

	// "aes" or "portable".
	const char* ImplName();

	// Identity of a byte stream. Independent of how Add is cut and of pointer alignment.
	class alignas(16) Stream
	{
	public:
		void Reset()
		{
			m_fill = 0;
			m_acc = kCombineInit;
			m_len = 0;
		}

		// Short Add is a memcpy into the buffer. Hash is the only out-of-line call.
		inline void Add(const u8* data, size_t size)
		{
			m_len += size;
			if (m_fill == 0 && size >= GS_BLOCK_SIZE)
			{
				if ((reinterpret_cast<uintptr_t>(data) & 15u) == 0)
				{
					do
					{
						m_acc = Combine(m_acc, Hash(data));
						data += GS_BLOCK_SIZE;
						size -= GS_BLOCK_SIZE;
					} while (size >= GS_BLOCK_SIZE);
				}
				else
				{
					do
					{
						std::memcpy(m_buf, data, GS_BLOCK_SIZE);
						m_acc = Combine(m_acc, Hash(m_buf));
						data += GS_BLOCK_SIZE;
						size -= GS_BLOCK_SIZE;
					} while (size >= GS_BLOCK_SIZE);
				}
			}
			while (size != 0)
			{
				const size_t space = GS_BLOCK_SIZE - m_fill;
				const size_t n = (size < space) ? size : space;
				std::memcpy(m_buf + m_fill, data, n);
				m_fill += static_cast<u32>(n);
				data += n;
				size -= n;
				if (m_fill == GS_BLOCK_SIZE)
				{
					m_acc = Combine(m_acc, Hash(m_buf));
					m_fill = 0;
				}
			}
		}

		u64 Finish()
		{
			if (m_fill != 0)
			{
				std::memset(m_buf + m_fill, 0, GS_BLOCK_SIZE - m_fill);
				m_acc = Combine(m_acc, Hash(m_buf));
				m_fill = 0;
			}
			return Combine(m_acc, m_len);
		}

	private:
		alignas(16) u8 m_buf[GS_BLOCK_SIZE];
		u32 m_fill = 0;
		u64 m_acc = kCombineInit;
		u64 m_len = 0;
	};

	// exposed for tests
	u64 HashPortable(const u8* block);
#if defined(ARCH_ARM64)
	u64 HashAES(const u8* block);
#endif
} // namespace GSBlockId
