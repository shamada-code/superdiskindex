///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "MFM.h"

u8 mfm_decode(u16 raw)
{
	int const o=0;
	u8 decval = (
		(((raw>>(o+14))&0x1)<<7) |
		(((raw>>(o+12))&0x1)<<6) |
		(((raw>>(o+10))&0x1)<<5) |
		(((raw>>(o+8))&0x1)<<4) |
		(((raw>>(o+6))&0x1)<<3) |
		(((raw>>(o+4))&0x1)<<2) |
		(((raw>>(o+2))&0x1)<<1) |
		(((raw>>(o+0))&0x1)<<0)
	);

	return decval;
}
