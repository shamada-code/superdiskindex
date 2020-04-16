///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "Buffer.h"
#include "Helpers.h"
#include "MFM.h"

///////////////////////////////////////////////////////////

void Buffer::RequireSize(u64 size)
{
	if (size>Size)
	{
		Alloc(Size*2);
	}
}

void Buffer::Alloc(u64 size)
{
	if (size<64) size=64;
	//if (Config.verbose>=3) printf("#BUFREALLOC: %d bytes\n", size);
	Data = (u8 *)realloc(Data, size);
	Size=size;
}

void Buffer::Push8(u8 db)
{
	RequireSize(Fill+1);
	Data[Fill++] = db;
}

void Buffer::Push16(u16 dw)
{
	RequireSize(Fill+2);
	Data[Fill++] = (dw>>8)&0xff;
	Data[Fill++] = (dw>>0)&0xff;
}

void Buffer::Push32(u32 dd)
{
	RequireSize(Fill+4);
	Data[Fill++] = (dd>>24)&0xff;
	Data[Fill++] = (dd>>16)&0xff;
	Data[Fill++] = (dd>>8)&0xff;
	Data[Fill++] = (dd>>0)&0xff;
}

void Buffer::Push64(u64 dl)
{
	RequireSize(Fill+8);
	Data[Fill++] = (dl>>56)&0xff;
	Data[Fill++] = (dl>>48)&0xff;
	Data[Fill++] = (dl>>40)&0xff;
	Data[Fill++] = (dl>>32)&0xff;
	Data[Fill++] = (dl>>24)&0xff;
	Data[Fill++] = (dl>>16)&0xff;
	Data[Fill++] = (dl>>8)&0xff;
	Data[Fill++] = (dl>>0)&0xff;
}

void Buffer::Clear()
{
	Fill=0;
}

void Buffer::MFMDecode()
{
	u8 *p8 = (u8 *)Data;
	u16 *p16 = (u16 *)Data;
	for (u32 i=0; i<(Fill/2); i++)
	{
		p8[i] = mfm_decode(swap16(p16[i]));
	}
	Fill = Fill/2;
}
