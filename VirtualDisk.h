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

///////////////////////////////////////////////////////////

struct VDSector
{
	u8 *Data;
};

struct VDHead
{
	VDSector *Sectors;
};

struct VDCyl
{
	VDHead *Heads;
};

struct VDDisk
{
	VDCyl *Cyls;
};

///////////////////////////////////////////////////////////

class VirtualDisk
{
public:
	VirtualDisk();
	virtual ~VirtualDisk();

	void SetLayout(u8 c, u8 h, u8 s, u16 ss);
	void AddSector(u8 c, u8 h, u8 s, void *p, u32 size);
	void *GetSector(u8 c, u8 h, u8 s);
	void *GetSector(u16 blk);

	void ExportADF(char const *fn);
	void ExportIMG(char const *fn);
	void ExportListing(char const *fn);
	void ExportState(char const *fn);

protected:
	u32 chs2blk(int c, int h, int s) { return c*Heads*Sects+h*Sects+s; }
	u32 blk2cyl(int blk) { return blk/(Sects*Heads); }
	u32 blk2head(int blk) { return (blk%(Sects*Heads))/Sects; }
	u32 blk2sect(int blk) { return (blk%(Sects*Heads))%Sects; }

	u8 Cyls;
	u8 Heads;
	u8 Sects;
	u16 SectSize;

	VDDisk Disk;
};
