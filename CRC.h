///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include "Helpers.h"

template <class T> class CRC_XOR
{
public:
	CRC_XOR(T initval) : State(initval) {}
	~CRC_XOR() {}

	////// Inputs

	// feed single values (stream mode)
	void Feed(T value, bool swap_endian)
	{
		if (swap_endian)
		{
			State ^= swap(value);
		} else {
			State ^= value;
		}
	}

	// feed buffer of data (block mode)
	// Note: count is a count of words, dependent on crc size!
	void Block(void *ptr, u32 count, bool swap_endian)
	{
		T *p = (T *)ptr;
		for (u32 i=0; i<count; i++)
		{
			if (swap_endian)
			{
				State ^= swap(p[i]);
			} else {
				State ^= p[i];
			}
		}
	}

	////// Outputs

	// Query last state for comparison
	T Get() { return State; }

	// Check whether state is zero
	bool Check() { return State==0; }

protected:
	T State;
};

typedef CRC_XOR<u8> CRC8_xor;
typedef CRC_XOR<u16> CRC16_xor;
typedef CRC_XOR<u32> CRC32_xor;

///////////////////////////////////////////////////////////////

template <class T> class CRC_CCITT
{
public:
	CRC_CCITT(T initval) : State(initval) {}
	~CRC_CCITT() {}

	////// Inputs

	// feed single values (stream mode)
	void Feed(T value, bool swap_endian, int force_input_width=0)
	{
		T input = swap_endian?swap(value):value;

		int bitsize = sizeof(T)*8;
		for (int i=(force_input_width>0?(force_input_width-1):(bitsize-1)); i>=0; i--)
		{
			FeedBit((input>>i)&0x1);
		}
	}

	// feed buffer of data (block mode)
	// Note: count is a count of words, dependent on crc size!
	void Block(void *ptr, u32 count, bool swap_endian)
	{
		T *p = (T *)ptr;
		for (u32 i=0; i<count; i++)
		{
			Feed(p[i],swap_endian);
		}
	}

	void FeedBit(u8 bit)
	{
		int bitsize = sizeof(T)*8;
		u8 f = bit^(State>>(bitsize-1));
		T fbits = (f<<5)|(f<<12)|(f<<0);
		State = (State<<1) ^ fbits;
	}

	////// Outputs

	// Query last state for comparison
	T Get() { return State; }

	// Check whether state is zero
	bool Check() { return State==0; }

protected:
	T State;
};

typedef CRC_CCITT<u16> CRC16_ccitt;
typedef CRC_CCITT<u32> CRC32_ccitt;
