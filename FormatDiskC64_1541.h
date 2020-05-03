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

class FormatDiskC64_1541 : public Format
{
public:
	FormatDiskC64_1541();
	virtual ~FormatDiskC64_1541();

	virtual char const *GetName();

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);
	virtual bool UsesGCR() { return true; }
	virtual u16 GetMaxExpectedCylinder();
	virtual u16 GetMaxExpectedSector();

	//virtual bool Detect();
	virtual void PreTrackInit();
	virtual void HandleBlock(Buffer *buffer, int currev);
	bool Analyze();

	virtual char const *GetDiskTypeString();
	virtual char const *GetDiskSubTypeString();

protected:
	//void ParseDirectory(int fd, u32 block, char const *prefix);
	//void ScanFile(u32 block);
	class DiskMap *DMap;
	class DiskLayout *DLayout;

	int cur_c,cur_h,cur_s;
	bool LayoutLocked;

	int ScanFile(u8 trk, u8 sect, int dir_blkcount, int cur_blkcount=0);
};
