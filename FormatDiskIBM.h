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

class FormatDiskIBM : public Format
{
public:
	FormatDiskIBM() {}
	virtual ~FormatDiskIBM() {}

	virtual char const *GetName();

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);

	//virtual bool Detect();
	virtual void HandleBlock(Buffer *buffer);
	virtual bool Analyze();

protected:
};
