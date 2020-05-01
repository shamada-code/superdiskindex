///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "Helpers.h"
#include "GCR.h"

u8 const __gcr_cbm_enc_map[] = {
	0x0a, // == encode(0x0)
	0x0b, // == encode(0x1)
	0x12, // == encode(0x2)
	0x13, // == encode(0x3)
	0x0e, // == encode(0x4)
	0x0f, // == encode(0x5)
	0x16, // == encode(0x6)
	0x17, // == encode(0x7)
	0x09, // == encode(0x8)
	0x19, // == encode(0x9)
	0x1a, // == encode(0xa)
	0x1b, // == encode(0xb)
	0x0d, // == encode(0xc)
	0x1d, // == encode(0xd)
	0x1e, // == encode(0xe)
	0x15, // == encode(0xf)
};

u8 const __gcr_cbm_dec_map[] = {
	0xff, // == decode(0x00)
	0xff, // == decode(0x01)
	0xff, // == decode(0x02)
	0xff, // == decode(0x03)
	0xff, // == decode(0x04)
	0xff, // == decode(0x05)
	0xff, // == decode(0x06)
	0xff, // == decode(0x07)
	0xff, // == decode(0x08)
	0x8,  // == decode(0x09)
	0x0,  // == decode(0x0a)
	0x1,  // == decode(0x0b)
	0xff, // == decode(0x0c)
	0xc,  // == decode(0x0d)
	0x4,  // == decode(0x0e)
	0x5,  // == decode(0x0f)
	0xff, // == decode(0x10)
	0xff, // == decode(0x11)
	0x2,  // == decode(0x12)
	0x3,  // == decode(0x13)
	0xff, // == decode(0x14)
	0xf,  // == decode(0x15)
	0x6,  // == decode(0x16)
	0x7,  // == decode(0x17)
	0xff, // == decode(0x18)
	0x9,  // == decode(0x19)
	0xa,  // == decode(0x1a)
	0xb,  // == decode(0x1b)
	0xff, // == decode(0x1c)
	0xd,  // == decode(0x1d)
	0xe,  // == decode(0x1e)
	0xff, // == decode(0x1f)
};

u8 gcr_cbm_encode(u8 value)
{
	if (value>0xf)
	{
		clog(1, "# GCR Encoding error. Out of bounds.\n");
		return 0;
	}
	return __gcr_cbm_enc_map[value];
}

u8 gcr_cbm_decode(u8 value)
{
	if (value>0x1f)
	{
		clog(1, "# GCR Decoding error. Out of bounds.\n");
		return 0;
	}
	if (__gcr_cbm_dec_map[value]==0xff)
	{
		//clog(1, "# GCR Decoding error. Invalid symbol.\n");
		return 0;
	}
	return __gcr_cbm_dec_map[value];
}
