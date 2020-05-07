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

struct VDRevolution
{
	u8 *Data;
	bool used;
	bool crc1ok;
	bool crc2ok;
};

struct VDSector
{
	VDRevolution *Revs;
	VDRevolution Merged;
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

	void SetLayout(u8 c, u8 h, u8 s, u8 r, u16 ss);
	u8 GetLayoutCylinders() { return Cyls; }
	u8 GetLayoutHeads() { return Heads; }
	u8 GetLayoutSectors() { return Sects; }
	u32 GetSectorCount() { return Cyls*Heads*Sects; }

	void AddSector(u8 c, u8 h, u8 s, u8 r, void *p, u32 size, bool crc1ok, bool crc2ok);
	void *GetSector(u8 c, u8 h, u8 s);
	void *GetSector(u16 blk);
	bool IsSectorMissing(u16 blk);
	bool IsSectorMissing(u16 c, u16 h, u16 s);
	bool IsSectorCRCBad(u16 blk);
	bool IsSectorCRCBad(u16 c, u16 h, u16 s);
	u32 GetMissingCount() { return SectorsMissing; }
	u32 GetCRCBadCount() { return SectorsCRCBad; }

	void MergeRevs(class Format *fmt);

	void ExportADF(char const *fn);
	void ExportIMG(char const *fn);
	void ExportD64(char const *fn);

protected:
	u32 chs2blk(int c, int h, int s) { return c*Heads*Sects+h*Sects+s; }
	u32 blk2cyl(int blk) { return blk/(Sects*Heads); }
	u32 blk2head(int blk) { return (blk%(Sects*Heads))/Sects; }
	u32 blk2sect(int blk) { return (blk%(Sects*Heads))%Sects; }

	u8 Cyls;
	u8 Heads;
	u8 Sects;
	u8 Revs;
	u16 SectSize;

	VDDisk Disk;
	class Buffer *FinalDisk;

	u32 SectorsMissing;
	u32 SectorsCRCBad;
};
