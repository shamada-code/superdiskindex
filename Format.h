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
	Format() : Disk(NULL),LastCyl(0),LastHead(1),LastSect(0) {}
	virtual ~Format() {}

	void SetVirtualDisk(class VirtualDisk *disk) { Disk=disk; }

	// called from BitStream
	virtual u8 GetSyncWordCount()=0;
	virtual u32 GetSyncWord(int n)=0;
	virtual u32 GetSyncBlockLen(int n)=0;

	//virtual bool Detect();
	virtual void HandleBlock(Buffer *buffer)=0;
	virtual bool Analyze()=0;

	u8 GetCyls() { return LastCyl+1; }
	u8 GetHeads() { return LastHead+1; }
	u8 GetSects() { return LastSect+1; }

protected:
	class VirtualDisk *Disk;
	u8 LastCyl;
	u8 LastHead;
	u8 LastSect;
};
