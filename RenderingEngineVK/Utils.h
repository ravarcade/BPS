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

	// copy of Phernost work from this link
	// https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
	class Float16Compressor
	{
		union Bits
		{
			float f;
			int32_t si;
			uint32_t ui;
		};

		static int const shift = 13;
		static int const shiftSign = 16;

		static int32_t const infN = 0x7F800000; // flt32 infinity
		static int32_t const maxN = 0x477FE000; // max flt16 normal as a flt32
		static int32_t const minN = 0x38800000; // min flt16 normal as a flt32
		static int32_t const signN = 0x80000000; // flt32 sign bit

		static int32_t const infC = infN >> shift;
		static int32_t const nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
		static int32_t const maxC = maxN >> shift;
		static int32_t const minC = minN >> shift;
		static int32_t const signC = signN >> shiftSign; // flt16 sign bit

		static int32_t const mulN = 0x52000000; // (1 << 23) / minN
		static int32_t const mulC = 0x33800000; // minN / (1 << (23 - shift))

		static int32_t const subC = 0x003FF; // max flt32 subnormal down shifted
		static int32_t const norC = 0x00400; // min flt32 normal down shifted

		static int32_t const maxD = infC - maxC - 1;
		static int32_t const minD = minC - subC - 1;

	public:

		static uint16_t compress(float value)
		{
			Bits v, s;
			v.f = value;
			uint32_t sign = v.si & signN;
			v.si ^= sign;
			sign >>= shiftSign; // logical shift
			s.si = mulN;
			s.si = static_cast<int32_t>(s.f * v.f); // correct subnormals
			v.si ^= (s.si ^ v.si) & -(minN > v.si);
			v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
			v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
			v.ui >>= shift; // logical shift
			v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
			v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
			return v.ui | sign;
		}

		static float decompress(uint16_t value)
		{
			Bits v;
			v.ui = value;
			int32_t sign = v.si & signC;
			v.si ^= sign;
			sign <<= shiftSign;
			v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
			v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
			Bits s;
			s.si = mulC;
			s.f *= v.si;
			int32_t mask = -(norC > v.si);
			v.si <<= shift;
			v.si ^= (s.si ^ v.si) & mask;
			v.si |= sign;
			return v.f;
		}
	};
};