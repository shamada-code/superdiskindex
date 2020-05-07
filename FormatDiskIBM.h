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

enum _FatType
{
	FAT12,
	FAT16,
	FAT32,
};

class FormatDiskIBM : public Format
{
public:
	FormatDiskIBM();
	virtual ~FormatDiskIBM() {}

	virtual char const *GetName();

	// called from BitStream
	virtual u8 GetSyncWordCount();
	virtual u32 GetSyncWord(int n);
	virtual u32 GetSyncBlockLen(int n);
	virtual bool UsesGCR() { return false; }
	virtual u16 GetMaxExpectedCylinder() { return 81; }
	virtual u16 GetMaxExpectedHead() { return 1; }
	virtual u16 GetMaxExpectedSector() { return 24; }
	virtual bool UsesVariableTrackLen() { return false; }

	//virtual bool Detect();
	virtual void PreTrackInit();
	virtual void HandleBlock(Buffer *buffer, int currev);
	virtual bool Analyze();

	virtual char const *GetDiskTypeString();
	virtual char const *GetDiskSubTypeString();

protected:
	void ParseDirectory(int fd, u32 block, u32 blkcount, char const *prefix, u32 *bad_sector_count);
	void ScanFile(u32 first_cluster, const char *fn, u32 *bad_sector_count);

	u32 cluster2sector(u32 cls);
	u32 clustersize();

	u32 GetFATChainNext(u32 cluster);

	void DetectFAT();
	enum _FatType FatType;
	u32 ClusterCount;
	char const *FatTypeName(_FatType fattype);

	int cur_c,cur_h,cur_s;
	//bool LayoutLocked;

	class DiskMap *DMap;
};
