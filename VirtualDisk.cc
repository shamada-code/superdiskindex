///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "VirtualDisk.h"
#include "Helpers.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

///////////////////////////////////////////////////////////

VirtualDisk::VirtualDisk()
{
}

VirtualDisk::~VirtualDisk()
{
}

void VirtualDisk::SetLayout(u8 c, u8 h, u8 s, u16 ss)
{ 
	Cyls=c; 
	Heads=h; 
	Sects=s;
	SectSize=ss; 

	Disk.Cyls = new VDCyl[c];
	for (int i=0; i<c; i++)
	{
		Disk.Cyls[i].Heads = new VDHead[h];
		for (int j=0; j<h; j++)
		{
			Disk.Cyls[i].Heads[j].Sectors = new VDSector[s];
			for (int k=0; k<s; k++)
			{
				Disk.Cyls[i].Heads[j].Sectors[k].Data = new u8[ss];
			}
		}
	}
}

void VirtualDisk::AddSector(u8 c, u8 h, u8 s, void *p, u32 size)
{
	if (Config.verbose>=1) printf("### Cyl %02d # Head %01d # Sect %02d\r", c,h,s);
	memcpy(Disk.Cyls[c].Heads[h].Sectors[s].Data, p, min(size,SectSize));
}

void *VirtualDisk::GetSector(u8 c, u8 h, u8 s)
{
	return Disk.Cyls[c].Heads[h].Sectors[s].Data;
}

void *VirtualDisk::GetSector(u16 blk)
{
	return Disk.Cyls[blk2cyl(blk)].Heads[blk2head(blk)].Sectors[blk2sect(blk)].Data;
}

void VirtualDisk::ExportADF(char const *fn)
{
	int fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);
	for (int c=0; c<Cyls; c++)
	{
		for (int h=0; h<Heads; h++)
		{
			for (int s=0; s<Sects; s++)
			{
				write(fd, Disk.Cyls[c].Heads[h].Sectors[s].Data, SectSize);
			}
		}
	}
	close(fd);
}

void VirtualDisk::ExportIMG(char const *fn)
{

}

void VirtualDisk::ExportListing(char const *fn)
{

}

void VirtualDisk::ExportState(char const *fn)
{

}
