///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include "Types.h"
#include "Format.h"
#include "Buffer.h"

///////////////////////////////////////////////////////////

class FormatDiskC64_1581 : public Format
{
public:
	FormatDiskC64_1581() {}
	virtual ~FormatDiskC64_1581() {}

	virtual char const *GetName();

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);
	virtual bool UsesGCR() { return false; }

	//virtual bool Detect();
	virtual void HandleBlock(Buffer *buffer, int currev);
	bool Analyze();

	virtual char const *GetDiskTypeString();
	virtual char const *GetDiskSubTypeString();

protected:
	//void ParseDirectory(int fd, u32 block, char const *prefix);
	//void ScanFile(u32 block);
	class DiskMap *DMap;
};
