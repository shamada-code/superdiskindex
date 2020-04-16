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
#include "Buffer.h"

class FormatDiskAmiga : public Format
{
public:
	FormatDiskAmiga() {}
	virtual ~FormatDiskAmiga() {}

	virtual char const *GetName();

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);

	//virtual bool Detect();
	virtual void HandleBlock(Buffer *buffer);
	bool Analyze();

	u16 Spread8(u8 data);
	u32 Spread16(u16 data);
	u64 Spread32(u32 data);

	u32 Weave16(u8 odd, u8 even);
	u32 Weave32(u16 odd, u16 even);

protected:
	void ParseDirectory(int fd, u32 block, char const *prefix);
};
