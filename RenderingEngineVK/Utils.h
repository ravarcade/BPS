#pragma once

namespace Utils {
	inline uint32_t trailing_zeroes(uint32_t x)
	{
		unsigned long result;
		if (_BitScanForward(&result, x))
			return result;
		else
			return 32;
	}

	template<typename T>
	inline void for_each_bit(uint32_t value, const T &func)
	{
		while (value)
		{
			uint32_t bit = trailing_zeroes(value);
			func(bit);
			value &= ~(1u << bit);
		}
	}

	inline uint32_t count_bits(uint32_t x)
	{
		x -= (x >> 1) & 0x55555555;                     //put count of each 2 bits into those 2 bits
		x = (x & 0x33333333) + ((x >> 2) & 0x33333333); //put count of each 4 bits into those 4 bits 
		x = (x + (x >> 4)) & 0x0f0f0f0f;                //put count of each 8 bits into those 8 bits 
		return (x * 0x01010101) >> 24;          //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
	}

	inline bool icasecmp(const std::string& l, const std::string& r)
	{
		return l.size() == r.size()
			&& equal(l.cbegin(), l.cend(), r.cbegin(),
				[](std::string::value_type l1, std::string::value_type r1)
		{ return toupper(l1) == toupper(r1); });
	}
};