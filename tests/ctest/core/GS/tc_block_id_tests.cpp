// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSBlockId.h"

#include <gtest/gtest.h>

#include <cstring>
#include <unordered_set>
#include <vector>

namespace
{
	using HashFn = u64 (*)(const u8*);

	void FillWord(u8* block, u32 word)
	{
		for (u32 i = 0; i < GS_BLOCK_SIZE; i += 4)
			std::memcpy(block + i, &word, 4);
	}

	u64 SplitMix(u64& state)
	{
		state += 0x9E3779B97F4A7C15ULL;
		u64 z = state;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	}

	void CheckHashFn(HashFn fn)
	{
		alignas(16) u8 a[GS_BLOCK_SIZE];
		alignas(16) u8 b[GS_BLOCK_SIZE];
		std::memset(a, 0xA5, sizeof(a));
		std::memcpy(b, a, sizeof(a));
		EXPECT_EQ(fn(a), fn(b));
		EXPECT_EQ(fn(a), fn(a));

		for (u32 i = 0; i < GS_BLOCK_SIZE; i++)
		{
			std::memcpy(b, a, sizeof(a));
			b[i] = static_cast<u8>(a[i] ^ 0xFF);
			EXPECT_NE(fn(a), fn(b)) << i;
		}

		const u32 bit_positions[] = {0, 1, 17, 127, 128, 255};
		for (u32 pos : bit_positions)
		{
			for (int bit = 0; bit < 8; bit++)
			{
				std::memcpy(b, a, sizeof(a));
				b[pos] = static_cast<u8>(a[pos] ^ static_cast<u8>(1u << bit));
				EXPECT_NE(fn(a), fn(b)) << pos << " bit " << bit;
			}
		}

		alignas(16) u8 zero[GS_BLOCK_SIZE];
		alignas(16) u8 ones[GS_BLOCK_SIZE];
		alignas(16) u8 eights[GS_BLOCK_SIZE];
		FillWord(zero, 0);
		FillWord(ones, 0xFFFFFFFFu);
		FillWord(eights, 0x80808080u);
		EXPECT_NE(fn(zero), fn(ones));
		EXPECT_NE(fn(zero), fn(eights));
		EXPECT_NE(fn(ones), fn(eights));

		std::unordered_set<u64> seen;
		seen.reserve(4096);
		u64 rng = 0xC0FFEE1234567890ULL;
		alignas(16) u8 block[GS_BLOCK_SIZE];
		for (int n = 0; n < 4096; n++)
		{
			for (u32 i = 0; i < GS_BLOCK_SIZE; i += 8)
			{
				const u64 w = SplitMix(rng);
				std::memcpy(block + i, &w, 8);
			}
			const u64 h = fn(block);
			EXPECT_TRUE(seen.insert(h).second) << n;
		}
	}

	u64 FeedStream(const u8* data, size_t size, size_t chunk)
	{
		GSBlockId::Stream st;
		st.Reset();
		if (size == 0)
			return st.Finish();
		if (chunk == 0 || chunk >= size)
		{
			st.Add(data, size);
			return st.Finish();
		}
		size_t off = 0;
		while (off < size)
		{
			size_t n = chunk;
			if (n > size - off)
				n = size - off;
			st.Add(data + off, n);
			off += n;
		}
		return st.Finish();
	}

	void FillPattern(u8* p, size_t size)
	{
		for (size_t i = 0; i < size; i++)
			p[i] = static_cast<u8>(i * 13u + 7u);
	}

	void CheckStreamSize(size_t size)
	{
		std::vector<u8> storage_a(size + 64);
		std::vector<u8> storage_b(size + 64);
		u8* aligned = storage_a.data();
		aligned += (16u - (reinterpret_cast<uintptr_t>(aligned) & 15u)) & 15u;
		u8* unaligned = storage_b.data();
		if ((reinterpret_cast<uintptr_t>(unaligned) & 15u) == 0)
			unaligned += 1;

		FillPattern(aligned, size);
		if (size != 0)
			std::memcpy(unaligned, aligned, size);

		const size_t chunks[] = {0, 1, 7, 30, 255, 256, 257};
		const u64 expected = FeedStream(aligned, size, 0);
		EXPECT_EQ(expected, FeedStream(aligned, size, 0));
		for (size_t chunk : chunks)
			EXPECT_EQ(expected, FeedStream(aligned, size, chunk)) << "size " << size << " chunk " << chunk;
		if (size != 0)
		{
			for (size_t chunk : chunks)
				EXPECT_EQ(expected, FeedStream(unaligned, size, chunk)) << "size " << size << " unaligned chunk " << chunk;

			const size_t positions[] = {0, size / 2, size - 1};
			std::vector<u8> flipped(size);
			for (size_t pos : positions)
			{
				std::memcpy(flipped.data(), aligned, size);
				flipped[pos] = static_cast<u8>(aligned[pos] ^ 0xFF);
				EXPECT_NE(expected, FeedStream(flipped.data(), size, 0)) << "size " << size << " pos " << pos;
			}
			if (size <= 257)
			{
				for (size_t pos = 0; pos < size; pos++)
				{
					std::memcpy(flipped.data(), aligned, size);
					flipped[pos] = static_cast<u8>(aligned[pos] ^ 0xFF);
					EXPECT_NE(expected, FeedStream(flipped.data(), size, 0)) << "size " << size << " pos " << pos;
				}
			}

			std::vector<u8> plus_zero(size + 1);
			std::memcpy(plus_zero.data(), aligned, size);
			plus_zero[size] = 0;
			EXPECT_NE(expected, FeedStream(plus_zero.data(), size + 1, 0)) << "size " << size;
		}
	}
} // namespace

TEST(GSBlockId, HashPortable)
{
	CheckHashFn(&GSBlockId::HashPortable);
}

TEST(GSBlockId, HashSelected)
{
	CheckHashFn(&GSBlockId::Hash);
}

#if defined(ARCH_ARM64)
TEST(GSBlockId, HashAES)
{
	CheckHashFn(&GSBlockId::HashAES);
}

TEST(GSBlockId, HashCRC)
{
	CheckHashFn(&GSBlockId::HashCRC);
}
#endif

TEST(GSBlockId, StreamCutsAndAlignment)
{
	const size_t sizes[] = {0, 1, 255, 256, 257, 512, 100, 100000};
	for (size_t size : sizes)
		CheckStreamSize(size);
}

TEST(GSBlockId, StreamEmptyStable)
{
	GSBlockId::Stream a;
	a.Reset();
	const u64 empty = a.Finish();
	GSBlockId::Stream b;
	b.Reset();
	EXPECT_EQ(empty, b.Finish());
	GSBlockId::Stream c;
	c.Reset();
	EXPECT_EQ(empty, c.Finish());
}

TEST(GSBlockId, CombineSensitive)
{
	const u64 a = 0x1111111111111111ULL;
	const u64 b = 0x2222222222222222ULL;
	const u64 c = 0x3333333333333333ULL;
	const u64 abc = GSBlockId::Combine(GSBlockId::Combine(GSBlockId::Combine(GSBlockId::kCombineInit, a), b), c);
	const u64 xbc = GSBlockId::Combine(GSBlockId::Combine(GSBlockId::Combine(GSBlockId::kCombineInit, a ^ 1), b), c);
	const u64 axc = GSBlockId::Combine(GSBlockId::Combine(GSBlockId::Combine(GSBlockId::kCombineInit, a), b ^ 1), c);
	const u64 abx = GSBlockId::Combine(GSBlockId::Combine(GSBlockId::Combine(GSBlockId::kCombineInit, a), b), c ^ 1);
	const u64 acb = GSBlockId::Combine(GSBlockId::Combine(GSBlockId::Combine(GSBlockId::kCombineInit, a), c), b);
	const u64 bac = GSBlockId::Combine(GSBlockId::Combine(GSBlockId::Combine(GSBlockId::kCombineInit, b), a), c);
	const u64 ab = GSBlockId::Combine(GSBlockId::Combine(GSBlockId::kCombineInit, a), b);
	EXPECT_NE(abc, xbc);
	EXPECT_NE(abc, axc);
	EXPECT_NE(abc, abx);
	EXPECT_NE(abc, acb);
	EXPECT_NE(abc, bac);
	EXPECT_NE(abc, ab);
}

TEST(GSBlockId, ImplNameKnown)
{
	const char* name = GSBlockId::ImplName();
	ASSERT_NE(name, nullptr);
	EXPECT_TRUE(std::strcmp(name, "aes") == 0 || std::strcmp(name, "crc") == 0 || std::strcmp(name, "portable") == 0);
}
