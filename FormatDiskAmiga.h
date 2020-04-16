///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include "Format.h"

class FormatDiskAmiga : public Format
{
public:
	FormatDiskAmiga() {}
	virtual ~FormatDiskAmiga() {}

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);

	//virtual bool Detect();
	virtual void HandleBlock(class Buffer *buffer);

	u16 Spread8(u8 data);
	u32 Spread16(u16 data);
	u64 Spread32(u32 data);

	u32 Weave16(u8 odd, u8 even);
	u32 Weave32(u16 odd, u16 even);

protected:
};
