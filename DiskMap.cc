///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "DiskMap.h"
#include "Helpers.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

///////////////////////////////////////////////////////////

DiskMap::DiskMap(u32 sectors, u32 clustersize_in_sectors, u32 first_cluster_offset)
{
	SectorCount=sectors;
	ClusterSize=clustersize_in_sectors;
	ClusterOffset=first_cluster_offset;
	
	Sectors = new u32[sectors];
	memset(Sectors, 0, sectors*sizeof(*Sectors));
}

DiskMap::~DiskMap()
{
	delete[](Sectors);
}

void DiskMap::SetBitsSector(u32 sector, u32 flags)
{
	if (sector>=SectorCount) { clog(2,"# ERR: Cannot mark sector %d in DiskMap (OOB).", sector); return; }
	Sectors[sector] |= flags;
}

void DiskMap::SetBitsCluster(u32 cluster, u32 flags)
{
	for (u32 i=0; i<ClusterSize; i++)
	{
		SetBitsSector(ClusterOffset+cluster*ClusterSize+i, flags);
	}
}

void DiskMap::ClearBitsSector(u32 sector, u32 flags)
{
	Sectors[sector] &= ~flags;
}

void DiskMap::ClearBitsCluster(u32 cluster, u32 flags)
{
	for (u32 i=0; i<ClusterSize; i++)
	{
		ClearBitsSector(ClusterOffset+cluster*ClusterSize+i, flags);
	}
}

u32 DiskMap::GetSector(u32 sector, u32 mask)
{
	if (sector>=SectorCount) return 0;
	return Sectors[sector]&mask;
}

void DiskMap::OutputMaps(char const *fn)
{
	int fd=-1;
	if (Config.gen_maps)
	{
		fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);
	}

	DPrintBlockMap(fd);
	DPrintFSMap(fd);
	DPrintHealthMap(fd);

	if (Config.gen_maps)
	{
		close(fd);
	}
}

void DiskMap::DPrintFSMap(int fd)
{
	cdprintf(Config.show_maps, Config.gen_maps, fd, "#################################################################################\n");
	cdprintf(Config.show_maps, Config.gen_maps, fd, "### FileSystemMap:\n");
	u16 outputwidth=128;
	u32 lines=SectorCount/outputwidth;
	if (lines*outputwidth<SectorCount) lines++;
	for (u32 i=0; i<lines; i++)
	{
		cdprintf(Config.show_maps, Config.gen_maps, fd, "# %04d | ", i*outputwidth);
		for (u32 j=0; (j<outputwidth)&&(i*outputwidth+j<SectorCount); j++)
		{
			char stype = '?';
			switch (GetSector(i*outputwidth+j, DMF_CONTENT_MASK))
			{
				case DMF_BOOTBLOCK: stype='B'; break;
				case DMF_ROOTBLOCK: stype='R'; break;
				case DMF_BLOCKMAP: stype='M'; break;
				case DMF_DIRECTORY: stype='D'; break;
				case DMF_FILEHEADER: stype='F'; break;
				case DMF_DATA: stype='+'; break;
				case 0: stype='.'; break;
			}
			cdprintf(Config.show_maps, Config.gen_maps, fd, "%c",stype);
		}
		cdprintf(Config.show_maps, Config.gen_maps, fd, "\n");
	}
	cdprintf(Config.show_maps, Config.gen_maps, fd, "# Legend: (.) Empty (B) Boot (R) Root (M) Blockmap/FAT (D) Directory (F) FileHeader (+) Data\n");
}

void DiskMap::DPrintBlockMap(int fd)
{
	cdprintf(Config.show_maps, Config.gen_maps, fd, "#################################################################################\n");
	cdprintf(Config.show_maps, Config.gen_maps, fd, "### BlockMap:\n");
	u16 outputwidth=128;
	u32 lines=SectorCount/outputwidth;
	if (lines*outputwidth<SectorCount) lines++;
	for (u32 i=0; i<lines; i++)
	{
		cdprintf(Config.show_maps, Config.gen_maps, fd, "# %04d | ", i*outputwidth);
		for (u32 j=0; (j<outputwidth)&&(i*outputwidth+j<SectorCount); j++)
		{
			char stype = '?';
			switch (GetSector(i*outputwidth+j, DMF_BLOCKMAP_MASK))
			{
				case 0: stype='.'; break;
				case DMF_BLOCK_USED: stype='+'; break;
				default: stype='?'; break;
			}
			cdprintf(Config.show_maps, Config.gen_maps, fd, "%c",stype);
		}
		cdprintf(Config.show_maps, Config.gen_maps, fd, "\n");
	}
	cdprintf(Config.show_maps, Config.gen_maps, fd, "# Legend: (.) Unused (+) Used\n");
}

void DiskMap::DPrintHealthMap(int fd)
{
	cdprintf(Config.show_maps, Config.gen_maps, fd, "#################################################################################\n");
	cdprintf(Config.show_maps, Config.gen_maps, fd, "### HealthMap:\n");
	u16 outputwidth=128;
	u32 lines=SectorCount/outputwidth;
	if (lines*outputwidth<SectorCount) lines++;
	for (u32 i=0; i<lines; i++)
	{
		cdprintf(Config.show_maps, Config.gen_maps, fd, "# %04d | ", i*outputwidth);
		for (u32 j=0; (j<outputwidth)&&(i*outputwidth+j<SectorCount); j++)
		{
			char stype = '?';
			u32 sstate = GetSector(i*outputwidth+j, DMF_HEALTH_MASK);
			if (false) {}
			else if ((sstate&DMF_MISSING)>0) stype='M';
			else if ((sstate&DMF_CRC_LOWLEVEL_BAD)>0) stype='D';
			else if ((sstate&DMF_CRC_HIGHLEVEL_BAD)>0) stype='C';
			else stype='.';
			cdprintf(Config.show_maps, Config.gen_maps, fd, "%c",stype);
		}
		cdprintf(Config.show_maps, Config.gen_maps, fd, "\n");
	}
	cdprintf(Config.show_maps, Config.gen_maps, fd, "# Legend: (.) Ok (M) Missing (D) Defect/Lowlevel crc bad (C) Corrupted (filesys crc bad)\n");
}
