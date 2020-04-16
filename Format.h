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

	//virtual bool Detect();
	virtual void HandleBlock(class Buffer *buffer)=0;

protected:
	class VirtualDisk *Disk;
};
