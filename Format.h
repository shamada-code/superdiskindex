///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include "Buffer.h"

class Format
{
public:
	Format() : Disk(NULL),LastCyl(0),LastHead(0),LastSect(0),SectSize(512) {}
	virtual ~Format() {}

	void SetVirtualDisk(class VirtualDisk *disk) { Disk=disk; }

	virtual char const *GetName()=0;

	// called from BitStream
	virtual u8 GetSyncWordCount()=0;
	virtual u32 GetSyncWord(int n)=0;
	virtual u32 GetSyncBlockLen(int n)=0;

	//virtual bool Detect();
	virtual void HandleBlock(Buffer *buffer, int currev)=0;
	virtual bool Analyze()=0;

	u8 GetCyls() { return LastCyl+1; }
	u8 GetHeads() { return LastHead+1; }
	u8 GetSects() { return LastSect+1; }
	u16 GetSectSize() { return SectSize; }

	void SetDiskType(u32 type) { DiskType=type; }
	void SetDiskSubType(u32 subtype) { DiskSubType=subtype; }

	u32 GetDiskType() { return DiskType; }
	u32 GetDiskSubType() { return DiskSubType; }

	virtual char const *GetDiskTypeString()=0;
	virtual char const *GetDiskSubTypeString()=0;

protected:
	class VirtualDisk *Disk;
	u8 LastCyl;
	u8 LastHead;
	u8 LastSect;
	u16 SectSize;

	u32 DiskType;
	u32 DiskSubType;
};

typedef Format *pFormat;
