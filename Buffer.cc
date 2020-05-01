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
#include "GCR.h"

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
	//clog(3,"#BUFREALLOC: %d bytes\n", size);
	Data = (u8 *)realloc(Data, size);
	Size=size;
}

void Buffer::SetFill(u64 fill)
{
	Fill=fill;
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

void Buffer::GCRDecode(u32 byteoffset, u8 bitoffset)
{
	Buffer b2;
	b2.Alloc(Size);
	u8 *p8 = ((u8 *)Data)+byteoffset;
	u8 p8bit = bitoffset;
	u8 *tgt = (u8 *)b2.Data;
	u8 tgtbit = 0;

	while (p8<(((u8 *)Data)+Fill))
	{
		// mask and get first part
		u8 bc1 = minval(5,8-p8bit);
		u8 mask1 = (0xff>>(8-bc1));
		u8 part1 = (*p8)>>((8-bc1)-p8bit);

		// advance pointer, if required
		p8bit+=bc1;
		if (p8bit>=8)
		{
			p8bit-=8;
			p8++;
		}

		// mask and get second part
		u8 bc2 = 5-bc1;
		u8 mask2 = (0xff>>(8-bc2));
		u8 part2 = (*p8)>>((8-bc2)-p8bit);

		// advance pointer, if required
		p8bit+=bc2;
		if (p8bit>=8)
		{
			p8bit-=8;
			p8++;
		}

		// merge parts and decode value
		u8 gcr_val = ((part1&mask1)<<bc2) | (part2&mask2);
		u8 dec_val = gcr_cbm_decode(gcr_val);
		//clog(3, "#GCR(%02x&%02x + %02x&%02x = %02x) = %02x\n", part1, mask1, part2, mask2, gcr_val, dec_val);

		// store decoded value
		(*tgt) = ((*tgt)&(0xf<<(tgtbit))) | (dec_val<<(4-tgtbit));
		tgtbit+=4;
		if (tgtbit>=8)
		{
			tgtbit-=8;
			tgt++;
		}
	}

	u32 decsize = tgt-(u8 *)b2.Data;
	memcpy(Data, b2.Data, decsize);
	Fill = decsize;
}
