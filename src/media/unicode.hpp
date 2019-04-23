#pragma once

#include <exception>
#include <cstdint>
#include <iterator>

namespace unicode
{
	enum class encoding { UTF8, UTF16BE, UTF16LE, UTF32BE, UTF32LE };

	typedef uint32_t codepoint;

	enum class except_e { RANGE, CORRUPT };

	class except : public std::exception
	{
	public:

		except(except_e ecode)
			: ecode(ecode)
		{
		}

		const char * what() const noexcept override
		{
			return ecode == except_e::CORRUPT ? "Corrupt character in UTF string" : "Premature end of UTF string";
		}

		const except_e ecode;
	};

	class utf8
	{
	public:

		template<class byte_iter>
		static unsigned int size(byte_iter begin, byte_iter end);

		static constexpr unsigned int size(codepoint point);

		template<class byte_iter>
		static byte_iter next(byte_iter begin, byte_iter end);

		template<class byte_iter>
		static byte_iter next(byte_iter begin);

		template<class byte_iter>
		static byte_iter decode(codepoint &point, byte_iter where, byte_iter end);

		template<class byte_iter>
		static byte_iter decode(codepoint &point, byte_iter where);

		template<class byte_iter>
		static byte_iter encode(codepoint point, byte_iter where, byte_iter end);

		template<class byte_iter>
		static void encode(codepoint point, byte_iter where);

		static constexpr unsigned int minsize = 1;

		static constexpr unsigned int maxsize = 4;

		static constexpr encoding type = encoding::UTF8;

	private:

		template<class byte_iter>
		static byte_iter encode_helper(codepoint point, byte_iter where, int bits);
	};

	template<bool be>
	class utf16
	{
	public:

		template<class byte_iter>
		static unsigned int size(byte_iter begin, byte_iter end);

		static constexpr unsigned int size(codepoint point);

		template<class byte_iter>
		static byte_iter next(byte_iter begin, byte_iter end);

		template<class byte_iter>
		static byte_iter next(byte_iter begin);

		template<class byte_iter>
		static byte_iter decode(codepoint &point, byte_iter where, byte_iter end);

		template<class byte_iter>
		static byte_iter decode(codepoint &point, byte_iter where);

		template<class byte_iter>
		static byte_iter encode(codepoint point, byte_iter where, byte_iter end);

		template<class byte_iter>
		static void encode(codepoint point, byte_iter where);

		static constexpr unsigned int minsize = 2;

		static constexpr unsigned int maxsize = 4;

		static constexpr encoding type = be ? encoding::UTF16BE : encoding::UTF16LE;
	};

	template<bool be>
	class utf32
	{
	public:

		template<class byte_iter>
		static unsigned int size(byte_iter begin, byte_iter end);

		static constexpr unsigned int size(codepoint point);

		template<class byte_iter>
		static byte_iter next(byte_iter begin, byte_iter end);

		template<class byte_iter>
		static byte_iter next(byte_iter begin);

		template<class byte_iter>
		static byte_iter decode(codepoint &point, byte_iter where, byte_iter end);

		template<class byte_iter>
		static byte_iter decode(codepoint &point, byte_iter where);

		template<class byte_iter>
		static byte_iter encode(codepoint point, byte_iter where, byte_iter end);

		template<class byte_iter>
		static void encode(codepoint point, byte_iter where);

		static constexpr unsigned int minsize = 4;

		static constexpr unsigned int maxsize = 4;

		static constexpr encoding type = be ? encoding::UTF32BE : encoding::UTF32LE;
	};

	/*
	 * Helper functions
	*/

	static inline const bool isbigendian()
	{
		const uint16_t test = 0xAABB;
		const unsigned char cmp[2] ={0xAA, 0xBB};
		return (reinterpret_cast<const unsigned char*>(&test)[0] == cmp[0]);
	}

	template<typename T>
	static inline T be2h(T value)
	{
		static_assert(sizeof(T) % 2 == 0 || sizeof(T) == 1);
		if constexpr (sizeof(T) == 1)
			return value;

		if (!isbigendian()) {
			for (auto i = 0, j = static_cast<int>(sizeof(T)-1); i < sizeof(T)/2; ++i, --j) {
				char tmp = reinterpret_cast<char*>(&value)[i];
				reinterpret_cast<char*>(&value)[i] = reinterpret_cast<char*>(&value)[j];
				reinterpret_cast<char*>(&value)[j] = tmp;
			}
		}
		return value;
	}

	template<typename T>
	static inline T h2be(T value)
	{
		return be2h(value);
	}

	template<typename T>
	static inline T le2h(T value)
	{
		static_assert(sizeof(T) % 2 == 0 || sizeof(T) == 1);
		if constexpr (sizeof(T) == 1)
			return value;

		if (isbigendian()) {
			for (auto i = 0, j = static_cast<int>(sizeof(T)-1); i < sizeof(T)/2; ++i, --j)
				reinterpret_cast<char*>(&value)[i] = reinterpret_cast<char*>(&value)[j];
		}
		return value;
	}

	template<typename T>
	static inline T h2le(T value)
	{
		return le2h(value);
	}

	/*
	 * Implementation
	*/

	template<class byte_iter>
	unsigned int utf8::size(byte_iter begin, byte_iter end)
	{
		if (begin != end) {
			// Count the amount of bytes
			int i;
			for (i = 0; i < 8; ++i) {
				if ((*begin & (0b1000'0000 >> i)) == 0)
					break;
			}

			if (i == 0)
				return 1;
			else if (i == 1 || i > 4)
				return 0;
			else
				return i;
		}
		return 0;
	}

	constexpr unsigned int utf8::size(codepoint point)
	{
		int bits = 0;
		for (int i = 0; i < 21; ++i) {
			if (point & (1 << i))
				bits = i+1;
		}

		if (bits <= 7)
			return 1;
		else if (bits <= 11)
			return 2;
		else if (bits <= 16)
			return 3;
		else
			return 4;
	}

	template<class byte_iter>
	byte_iter utf8::next(byte_iter begin, byte_iter end)
	{
		if (begin != end) {
			// Count the amount of bytes
			int i;
			for (i = 0; i < 8; ++i) {
				if ((*begin & (0b1000'0000 >> i)) == 0)
					break;
			}

			// Go to the next character and check for EOF
			++begin;
			if (begin == end) {
				if (i != 0) {
					if (i == 1 || i > 4)
						throw except(except_e::CORRUPT);
					else
						throw except(except_e::RANGE);
				}
				return begin;
			}
			if (i > 4)
				throw except(except_e::CORRUPT);

			// Check the next bytes
			if (i > 0) {
				int j = 1;
				while (begin != end) {
					if ((*begin & 0b1100'0000) != 0b1000'0000)
						break;
					if (j > i)
						throw except(except_e::CORRUPT);
					else
						++begin; ++j;
				}

				if (begin == end && i != j)
					throw except(except_e::RANGE);
			}
		}
		return begin;
	}

	template<class byte_iter>
	byte_iter utf8::next(byte_iter begin)
	{
		// Count the amount of bytes
		int i;
		for (i = 0; i < 8; ++i) {
			if ((*begin & (0b1000'0000 >> i)) == 0)
				break;
		}
		++begin;
		if (i > 4)
			throw except(except_e::CORRUPT);

		// Check the next bytes
		if (i > 0) {
			int j = 1;
			while (true) {
				if ((*begin & 0b1100'0000) != 0b1000'0000)
					break;
				
				if (j > i)
					throw except(except_e::CORRUPT);
				else
					++begin; ++j;
			}
		}
		return begin;
	}

	template<class byte_iter>
	byte_iter utf8::decode(codepoint &point, byte_iter where, byte_iter end)
	{
		if (where == end)
			throw except(except_e::RANGE);

		int size;
		for (size = 0; size < 8; ++size) {
			if ((*where & (0b1000'0000 >> size)) == 0)
				break;
		}
		if (size > 4 || size == 1)
			throw except(except_e::CORRUPT);
		if (size == 0)
			size = 1;

		codepoint out = 0;
		if (isbigendian()) {
			switch (size) {
			case 1:
				reinterpret_cast<char*>(&out)[3] = (*where);
				break;
			case 2:
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00011100) >> 2;
				reinterpret_cast<char*>(&out)[3]  = (*where & 0b00000011) << 6;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[3] |= (*where & 0b00111111);
				break;
			case 3:
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00001111) << 4;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[2] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[3]  = (*where & 0b00000011) << 6;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[3] |= (*where & 0b00111111);
				break;
			case 4:
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00000111) << 2;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[1] |= (*where & 0b00110000) >> 4;
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00001111) << 4;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[2] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[3]  = (*where & 0b00000011) << 6;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[3] |= (*where & 0b00111111);
				break;
			}
		}
		else {
			switch (size) {
			case 1:
				reinterpret_cast<char*>(&out)[0] = (*where);
				break;
			case 2:
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00011100) >> 2;
				reinterpret_cast<char*>(&out)[0]  = (*where & 0b00000011) << 6;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[0] |= (*where & 0b00111111);
				break;
			case 3:
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00001111) << 4;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[1] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[0]  = (*where & 0b00000011) << 6;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[0] |= (*where & 0b00111111);
				break;
			case 4:
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00000111) << 2;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[2] |= (*where & 0b00110000) >> 4;
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00001111) << 4;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[1] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[0]  = (*where & 0b00000011) << 6;		++where;
				if (where == end)
					throw except(except_e::RANGE);
				reinterpret_cast<char*>(&out)[0] |= (*where & 0b00111111);
				break;
			}
		}

		point = out;
		if (point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF))
			throw except(except_e::CORRUPT);
		return ++where;
	}

	template<class byte_iter>
	inline byte_iter utf8::decode(codepoint& point, byte_iter where)
	{
		int size;
		for (size = 0; size < 8; ++size) {
			if ((*where & (0b1000'0000 >> size)) == 0)
				break;
		}
		if (size > 4 || size == 1)
			throw except(except_e::CORRUPT);
		if (size == 0)
			size = 1;

		codepoint out = 0;
		if (isbigendian()) {
			switch (size) {
			case 1:
				reinterpret_cast<char*>(&out)[3] = (*where);
				break;
			case 2:
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00011100) >> 2;
				reinterpret_cast<char*>(&out)[3]  = (*where & 0b00000011) << 6;		++where;
				reinterpret_cast<char*>(&out)[3] |= (*where & 0b00111111);
				break;
			case 3:
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00001111) << 4;		++where;
				reinterpret_cast<char*>(&out)[2] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[3]  = (*where & 0b00000011) << 6;		++where;
				reinterpret_cast<char*>(&out)[3] |= (*where & 0b00111111);
				break;
			case 4:
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00000111) << 2;		++where;
				reinterpret_cast<char*>(&out)[1] |= (*where & 0b00110000) >> 4;
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00001111) << 4;		++where;
				reinterpret_cast<char*>(&out)[2] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[3]  = (*where & 0b00000011) << 6;		++where;
				reinterpret_cast<char*>(&out)[3] |= (*where & 0b00111111);
				break;
			}
		}
		else {
			switch (size) {
			case 1:
				reinterpret_cast<char*>(&out)[0] = (*where);
				break;
			case 2:
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00011100) >> 2;
				reinterpret_cast<char*>(&out)[0]  = (*where & 0b00000011) << 6;		++where;
				reinterpret_cast<char*>(&out)[0] |= (*where & 0b00111111);
				break;
			case 3:
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00001111) << 4;		++where;
				reinterpret_cast<char*>(&out)[1] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[0]  = (*where & 0b00000011) << 6;		++where;
				reinterpret_cast<char*>(&out)[0] |= (*where & 0b00111111);
				break;
			case 4:
				reinterpret_cast<char*>(&out)[2]  = (*where & 0b00000111) << 2;		++where;
				reinterpret_cast<char*>(&out)[2] |= (*where & 0b00110000) >> 4;
				reinterpret_cast<char*>(&out)[1]  = (*where & 0b00001111) << 4;		++where;
				reinterpret_cast<char*>(&out)[1] |= (*where & 0b00111100) >> 2;
				reinterpret_cast<char*>(&out)[0]  = (*where & 0b00000011) << 6;		++where;
				reinterpret_cast<char*>(&out)[0] |= (*where & 0b00111111);
				break;
			}
		}

		point = out;
		if (point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF))
			throw except(except_e::CORRUPT);
		return ++where;
	}

	template<class byte_iter>
	byte_iter utf8::encode(codepoint point, byte_iter where, byte_iter end)
	{
		if (where != end) {
			int bits = 0;
			for (int i = 0; i < 21; ++i) {
				if (point & (1 << i))
					bits = i+1;
			}

			if (bits <= 7 || (bits > 7 && bits <= 11 && std::distance(where, end) >= 2) || (bits > 11 && bits <= 16 && std::distance(where, end) >= 3)
				|| (bits > 16 && std::distance(where, end) >= 4))
			{
				auto val = encode_helper(point, where, bits);
				return ++val;
			}
		}
		throw except(except_e::RANGE);
	}

	template<class byte_iter>
	void utf8::encode(codepoint point, byte_iter where)
	{
		int bits = 0;
		for (int i = 0; i < 21; ++i) {
			if (point & (1 << i))
				bits = i+1;
		}

		encode_helper(point, where, bits);
	}

	template<class byte_iter>
	byte_iter utf8::encode_helper(codepoint point, byte_iter where, int bits)
	{
		if (point <= 0x10FFFF && (point < 0xD800 || point > 0xDFFF)) {
			if (isbigendian()) {
				if (bits <= 7) {
					*where = (reinterpret_cast<char*>(&point)[3]);
					return where;
				}
				else if (bits <= 11) {
					*where  = ((reinterpret_cast<char*>(&point)[2] & 0b0000'0111) << 2)		| 0b1100'0000 
							| ((reinterpret_cast<char*>(&point)[3] & 0b1100'0000) >> 6);					++where;
					*where  =  (reinterpret_cast<char*>(&point)[3] & 0b0011'1111)			| 0b1000'0000;
					return where;
				}
				else if (bits <= 16) {
					*where  = ((reinterpret_cast<char*>(&point)[2] & 0b1111'0000) >> 4)		| 0b1110'0000;	++where;
					*where  = ((reinterpret_cast<char*>(&point)[2] & 0b0000'1111) << 2)		| 0b1000'0000
							| ((reinterpret_cast<char*>(&point)[3] & 0b1100'0000) >> 6);					++where;
					*where  =  (reinterpret_cast<char*>(&point)[3] & 0b0011'1111)			| 0b1000'0000;
					return where;
				}
				else {
					*where  = ((reinterpret_cast<char*>(&point)[1] & 0b0001'1100) >> 2)		| 0b1111'0000;	++where;
					*where  = ((reinterpret_cast<char*>(&point)[1] & 0b0000'0011) << 4)		| 0b1000'0000
							| ((reinterpret_cast<char*>(&point)[2] & 0b1111'0000) >> 4);					++where;
					*where  = ((reinterpret_cast<char*>(&point)[2] & 0b0000'1111) << 2)		| 0b1000'0000
							| ((reinterpret_cast<char*>(&point)[3] & 0b1100'0000) >> 6);					++where;
					*where  =  (reinterpret_cast<char*>(&point)[3] & 0b0011'1111)			| 0b1000'0000;
					return where;
				}
			}
			else {
				if (bits <= 7) {
					*where = (reinterpret_cast<char*>(&point)[0]);
					return where;
				}
				else if (bits <= 11) {
					*where  = ((reinterpret_cast<char*>(&point)[1] & 0b0000'0111) << 2)		| 0b1100'0000
							| ((reinterpret_cast<char*>(&point)[0] & 0b1100'0000) >> 6);					++where;
					*where  =  (reinterpret_cast<char*>(&point)[0] & 0b0011'1111)			| 0b1000'0000;
					return where;
				}
				else if (bits <= 16) {
					*where  = ((reinterpret_cast<char*>(&point)[1] & 0b1111'0000) >> 4)		| 0b1110'0000;	++where;
					*where  = ((reinterpret_cast<char*>(&point)[1] & 0b0000'1111) << 2)		| 0b1000'0000
							| ((reinterpret_cast<char*>(&point)[0] & 0b1100'0000) >> 6);					++where;
					*where  =  (reinterpret_cast<char*>(&point)[0] & 0b0011'1111)			| 0b1000'0000;
					return where;
				}
				else {
					*where  = ((reinterpret_cast<char*>(&point)[2] & 0b0001'1100) >> 2)		| 0b1111'0000;	++where;
					*where  = ((reinterpret_cast<char*>(&point)[2] & 0b0000'0011) << 4)		| 0b1000'0000
							| ((reinterpret_cast<char*>(&point)[1] & 0b1111'0000) >> 4);					++where;
					*where  = ((reinterpret_cast<char*>(&point)[1] & 0b0000'1111) << 2)		| 0b1000'0000
							| ((reinterpret_cast<char*>(&point)[0] & 0b1100'0000) >> 6);					++where;
					*where  =  (reinterpret_cast<char*>(&point)[0] & 0b0011'1111)			| 0b1000'0000;
					return where;
				}
			}
		}
		throw except(except_e::CORRUPT);
	}

	template<bool be>
	template<class byte_iter>
	unsigned int utf16<be>::size(byte_iter begin, byte_iter end)
	{
		if (begin != end) {
			// Grab first value
			uint16_t value;
			reinterpret_cast<char*>(&value)[0] = *begin++;
			if (begin == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[1] = *begin++;

			// Check the first value
			value = be ? be2h(value) : le2h(value);
			if (value <= 0xD7FF || value >=  0xE000)
				return 2;
			if (begin == end)
				throw except(except_e::RANGE);

			// Grab the second value
			uint16_t value2;
			reinterpret_cast<char*>(&value2)[0] = *begin++;
			if (begin == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value2)[1] = *begin++;

			// Check the second value
			value2 = be ? be2h(value) : le2h(value);
			return (((value & 0b11111100'00000000) ^ 0b11011000'00000000) != 0 || ((value2 & 0b11111100'00000000) ^ 0b11011100'00000000) != 0) ? 0 : 4;
		}
	}

	template<bool be>
	unsigned constexpr int utf16<be>::size(codepoint point)
	{
		return (point <= 0xD7FF || point >=  0xE000) ? 2 : 4;
	}

	template<bool be>
	template<class byte_iter>
	byte_iter utf16<be>::next(byte_iter begin, byte_iter end)
	{
		if (begin != end) {
			// Grab first value
			uint16_t value;
			reinterpret_cast<char*>(&value)[0] = *begin++;
			if (begin == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[1] = *begin++;
			
			// Check the first value
			value = be ? be2h(value) : le2h(value);
			if (value <= 0xD7FF || value >=  0xE000)
				return begin;
			if (begin == end)
				throw except(except_e::RANGE);

			// Grab the second value
			uint16_t value2;
			reinterpret_cast<char*>(&value2)[0] = *begin++;
			if (begin == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value2)[1] = *begin++;

			// Check the second value
			value2 = be ? be2h(value) : le2h(value);
			if (((value & 0b11111100'00000000) ^ 0b11011000'00000000) != 0 || ((value2 & 0b11111100'00000000) ^ 0b11011100'00000000) != 0)
				throw except(except_e::CORRUPT);
		}
		return begin;
	}

	template<bool be>
	template<class byte_iter>
	inline byte_iter utf16<be>::next(byte_iter begin)
	{
		// Check the first value
		uint16_t value;
		reinterpret_cast<char*>(&value)[0] = *begin++;
		reinterpret_cast<char*>(&value)[1] = *begin++;
		value = be ? be2h(value) : le2h(value);
		if (value <= 0xD7FF || value >=  0xE000)
			return begin;

		// Check the second value
		uint16_t value2;
		reinterpret_cast<char*>(&value2)[0] = *begin++;
		reinterpret_cast<char*>(&value2)[1] = *begin++;
		value2 = be ? be2h(value) : le2h(value);
		if (((value & 0b11111100'00000000) ^ 0b11011000'00000000) != 0 || ((value2 & 0b11111100'00000000) ^ 0b11011100'00000000) != 0)
			throw except(except_e::CORRUPT);

		return begin;
	}

	template<bool be>
	template<class byte_iter>
	byte_iter utf16<be>::decode(codepoint &point, byte_iter where, byte_iter end)
	{
		if (where == end)
			throw except(except_e::RANGE);

		// Grab first value
		uint16_t value;
		reinterpret_cast<char*>(&value)[0] = *where++;
		if (where == end)
			throw except(except_e::RANGE);
		reinterpret_cast<char*>(&value)[1] = *where++;

		// Check the first value
		value = be ? be2h(value) : le2h(value);
		if (value <= 0xD7FF || value >=  0xE000) {
			point = value;
			return where;
		}
		if (where == end)
			throw except(except_e::RANGE);

		// Grab the second value
		uint16_t value2;
		reinterpret_cast<char*>(&value2)[0] = *where++;
		if (where == end)
			throw except(except_e::RANGE);
		reinterpret_cast<char*>(&value2)[1] = *where++;

		// Check the second value
		value2 = be ? be2h(value) : le2h(value);
		if (((value & 0b11111100'00000000) ^ 0b11011000'00000000) != 0 || ((value2 & 0b11111100'00000000) ^ 0b11011100'00000000) != 0)
			throw except(except_e::CORRUPT);

		point = ((value-0xD800) * 0x400) + (value2-0xDC00) +  0x10000;

		return where;
	}

	template<bool be>
	template<class byte_iter>
	inline byte_iter utf16<be>::decode(codepoint &point, byte_iter where)
	{
		// Check the first value
		uint16_t value;
		reinterpret_cast<char*>(&value)[0] = *where++;
		reinterpret_cast<char*>(&value)[1] = *where++;
		value = be ? be2h(value) : le2h(value);
		if (value <= 0xD7FF || value >=  0xE000) {
			point = value;
			return where;
		}

		// Check the second value
		uint16_t value2;
		reinterpret_cast<char*>(&value2)[0] = *where++;
		reinterpret_cast<char*>(&value2)[1] = *where++;
		value2 = be ? be2h(value) : le2h(value);
		if (((value & 0b11111100'00000000) ^ 0b11011000'00000000) != 0 || ((value2 & 0b11111100'00000000) ^ 0b11011100'00000000) != 0)
			throw except(except_e::CORRUPT);

		point = ((value-0xD800) * 0x400) + (value2-0xDC00) +  0x10000;

		return where;
	}

	template<bool be>
	template<class byte_iter>
	byte_iter utf16<be>::encode(codepoint point, byte_iter where, byte_iter end)
	{
		if ((point < 0xD800 && std::distance(where, end) < 2) || (point >= 0xD800 && std::distance(where, end) < 4)) {
			throw except(except_e::RANGE);
		}
		else {
			if (point <= 0x10FFFF && (point < 0xD800 || point > 0xDFFF)) {
				if (point < 0xD800) {
					uint16_t buffer = be ? h2be(static_cast<uint16_t>(point)) : h2le(static_cast<uint16_t>(point));
					*where = reinterpret_cast<char*>(&buffer)[0]; ++where;
					*where = reinterpret_cast<char*>(&buffer)[1]; ++where;
					return where;
				}
				else {
					uint16_t high = ((point - 0x10000) / 0x400) + 0xD800;
					uint16_t low = ((point - 0x10000) % 0x400) + 0xDC00;
					high = be ? h2be(high) : h2le(high);
					low = be ? h2be(low) : h2le(low);
					*where = reinterpret_cast<char*>(&high)[0]; ++where;
					*where = reinterpret_cast<char*>(&high)[1]; ++where;
					*where = reinterpret_cast<char*>(&low)[0]; ++where;
					*where = reinterpret_cast<char*>(&low)[1]; ++where;
					return where;
				}
			}
			throw except(except_e::CORRUPT);
		}
	}

	template<bool be>
	template<class byte_iter>
	void utf16<be>::encode(codepoint point, byte_iter where)
	{
		if (point <= 0x10FFFF && (point < 0xD800 || point > 0xDFFF)) {
			if (point < 0xD800) {
				uint16_t buffer = be ? h2be(static_cast<uint16_t>(point)) : h2le(static_cast<uint16_t>(point));
				*where = reinterpret_cast<char*>(&buffer)[0]; ++where;
				*where = reinterpret_cast<char*>(&buffer)[1];
				return;
			}
			else {
				uint16_t high = ((point - 0x10000) / 0x400) + 0xD800;
				uint16_t low = ((point - 0x10000) % 0x400) + 0xDC00;
				high = be ? h2be(high) : h2le(high);
				low = be ? h2be(low) : h2le(low);
				*where = reinterpret_cast<char*>(&high)[0]; ++where;
				*where = reinterpret_cast<char*>(&high)[1]; ++where;
				*where = reinterpret_cast<char*>(&low)[0]; ++where;
				*where = reinterpret_cast<char*>(&low)[1];
				return;
			}
		}
		throw except(except_e::CORRUPT);
	}

	template<bool be>
	template<class byte_iter>
	unsigned int utf32<be>::size(byte_iter begin, byte_iter end)
	{
		if (begin != end) {
			return (std::distance(begin, end) < 4) ? 0 : 4;
		}
		return 0;
	}

	template<bool be>
	unsigned constexpr int utf32<be>::size(codepoint point)
	{
		return 4;
	}

	template<bool be>
	template<class byte_iter>
	byte_iter utf32<be>::next(byte_iter begin, byte_iter end)
	{
		if (begin != end) {
			codepoint value;
			reinterpret_cast<char*>(&value)[0] = *begin++;
			if (begin == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[1] = *begin++;
			if (begin == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[2] = *begin++;
			if (begin == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[3] = *begin++;
		}
		return begin;
	}

	template<bool be>
	template<class byte_iter>
	inline byte_iter utf32<be>::next(byte_iter begin)
	{
		codepoint value;
		reinterpret_cast<char*>(&value)[0] = *begin++;
		reinterpret_cast<char*>(&value)[1] = *begin++;
		reinterpret_cast<char*>(&value)[2] = *begin++;
		reinterpret_cast<char*>(&value)[3] = *begin++;
		return begin;
	}

	template<bool be>
	template<class byte_iter>
	byte_iter utf32<be>::decode(codepoint &point, byte_iter where, byte_iter end)
	{
		if (where != end) {
			codepoint value;
			reinterpret_cast<char*>(&value)[0] = *where++;
			if (where == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[1] = *where++;
			if (where == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[2] = *where++;
			if (where == end)
				throw except(except_e::RANGE);
			reinterpret_cast<char*>(&value)[3] = *where++;
			value = be ? be2h(value) : le2h(value);
			if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
				throw except(except_e::CORRUPT);
			point = value;
			return where;
		}
		throw except(except_e::RANGE);
	}

	template<bool be>
	template<class byte_iter>
	inline byte_iter utf32<be>::decode(codepoint& point, byte_iter where)
	{
		codepoint value;
		reinterpret_cast<char*>(&value)[0] = *where++;
		reinterpret_cast<char*>(&value)[1] = *where++;
		reinterpret_cast<char*>(&value)[2] = *where++;
		reinterpret_cast<char*>(&value)[3] = *where++;
		value = be ? be2h(value) : le2h(value);
		if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
			throw except(except_e::CORRUPT);
		return where;
	}

	template<bool be>
	template<class byte_iter>
	byte_iter utf32<be>::encode(codepoint point, byte_iter where, byte_iter end)
	{
		if (std::distance(where, end) < 4) {
			throw except(except_e::RANGE);
		}
		else {
			if (point <= 0x10FFFF && (point < 0xD800 || point > 0xDFFF)) {
				point = be ? h2be(point) : h2le(point);
				*where = reinterpret_cast<char*>(&point)[0]; ++where;
				*where = reinterpret_cast<char*>(&point)[1]; ++where;
				*where = reinterpret_cast<char*>(&point)[2]; ++where;
				*where = reinterpret_cast<char*>(&point)[3]; ++where;
				return where;
			}
			throw except(except_e::CORRUPT);
		}
	}

	template<bool be>
	template<class byte_iter>
	void utf32<be>::encode(codepoint point, byte_iter where)
	{
		if (point <= 0x10FFFF && (point < 0xD800 || point > 0xDFFF)) {
			point = be ? h2be(point) : h2le(point);
			*where = reinterpret_cast<char*>(&point)[0]; ++where;
			*where = reinterpret_cast<char*>(&point)[1]; ++where;
			*where = reinterpret_cast<char*>(&point)[2]; ++where;
			*where = reinterpret_cast<char*>(&point)[3];
			return;
		}
		throw except(except_e::CORRUPT);
	}
}