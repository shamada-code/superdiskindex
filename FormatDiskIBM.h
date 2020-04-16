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

class FormatDiskIBM : public Format
{
public:
	FormatDiskIBM() {}
	virtual ~FormatDiskIBM() {}

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);

	//virtual bool Detect();
	virtual void HandleBlock(class Buffer *buffer);

protected:
};
