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
	FormatDiskIBM() : cur_c(-1),cur_h(-1),cur_s(-1) {}
	virtual ~FormatDiskIBM() {}

	virtual char const *GetName();

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);

	//virtual bool Detect();
	virtual void HandleBlock(Buffer *buffer, int currev);
	virtual bool Analyze();

protected:
	void ParseDirectory(int fd, u32 block, u32 blkcount, char const *prefix);

	int cur_c,cur_h,cur_s;
};
