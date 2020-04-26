///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

enum DiskMapFlags
{
	DMF_CRC_LOWLEVEL_BAD 	= 0x0001,
	DMF_CRC_HIGHLEVEL_BAD = 0x0002,
	DMF_MISSING			 			= 0x0004,
	DMF_HEALTH_MASK				= 0x000f,

	DMF_BOOTBLOCK 				= 0x0010,
	DMF_ROOTBLOCK 				= 0x0020,
	DMF_BLOCKMAP 					= 0x0040,
	DMF_FAT			 					= 0x0040, // this can be considered an alias for the blockmap
	DMF_DIRECTORY 				= 0x0080,
	DMF_FILEHEADER				= 0x0100,
	DMF_DATA							= 0x0200,
	DMF_CONTENT_MASK			= 0x0ff0,
};

class DiskMap
{
public:
	DiskMap(u32 sectors, u32 clustersize_in_sectors=1, u32 first_cluster_offset=0);
	virtual ~DiskMap();

	void SetBitsSector(u32 sector, u32 flags);
	void SetBitsCluster(u32 cluster, u32 flags);
	void ClearBitsSector(u32 sector, u32 flags);
	void ClearBitsCluster(u32 cluster, u32 flags);

	u32 GetSector(u32 sector, u32 mask=~0);

	void OutputMaps(char const *fn);

protected:
	void DPrintFSMap(int fd);
	void DPrintBlockMap(int fd);
	void DPrintHealthMap(int fd);

	u32 SectorCount;
	u32 ClusterSize;
	u32 ClusterOffset;

	u32 *Sectors;
};
