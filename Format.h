///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

class Format
{
public:
	Format() : Disk(NULL) {}
	virtual ~Format() {}

	void SetVirtualDisk(class VirtualDisk *disk) { Disk=disk; }

	// called from BitStream
	virtual u8 GetSyncWordCount()=0;
	virtual u32 GetSyncWord(int n)=0;
	virtual u32 GetSyncBlockLen(int n)=0;

	//virtual bool Detect();
	virtual void HandleBlock(class Buffer *buffer)=0;

protected:
	class VirtualDisk *Disk;
};
