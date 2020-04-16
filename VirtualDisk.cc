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

void VirtualDisk::SetLayout(u8 c, u8 h, u8 s, u8 r, u16 ss)
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
				Disk.Cyls[i].Heads[j].Sectors[k].Revs = new VDRevolution[r];
				for (int l=0; l<r; l++)
				{
					Disk.Cyls[i].Heads[j].Sectors[k].Revs[l].Data = new u8[ss];
					Disk.Cyls[i].Heads[j].Sectors[k].Revs[l].used=false;
					Disk.Cyls[i].Heads[j].Sectors[k].Revs[l].crc1ok=false;
					Disk.Cyls[i].Heads[j].Sectors[k].Revs[l].crc2ok=false;
				}
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.Data = new u8[ss];
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.used=false;
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.crc1ok=false;
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.crc2ok=false;
			}
		}
	}
}

void VirtualDisk::AddSector(u8 c, u8 h, u8 s, u8 r, void *p, u32 size, bool crc1ok, bool crc2ok)
{
	if (Config.verbose>=1) printf("### Cyl %02d # Head %01d # Sect %02d\r", c,h,s);
	memcpy(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, p, min(size,SectSize));
	Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used=true;
	Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok=crc1ok;
	Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok=crc2ok;
}

void *VirtualDisk::GetSector(u8 c, u8 h, u8 s)
{
	return Disk.Cyls[c].Heads[h].Sectors[s].Merged.Data;
}

void *VirtualDisk::GetSector(u16 blk)
{
	return Disk.Cyls[blk2cyl(blk)].Heads[blk2head(blk)].Sectors[blk2sect(blk)].Merged.Data;
}

void VirtualDisk::MergeRevs()
{
	printf("# Merging sector copies.\n");
	for (int c=0; c<Cyls; c++)
	{
		for (int h=0; h<Heads; h++)
		{
			for (int s=0; s<Sects; s++)
			{
				bool ok=false;
				for (int r=0; r<Revs; r++)
				{
					if ( 
						(!ok) &&
						(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used) &&
						(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok) &&
						(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok) )
					{
						memcpy(Disk.Cyls[c].Heads[h].Sectors[s].Merged.Data, Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, SectSize);
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.used = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc1ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc2ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok;
						ok=true;
					}
				}
				for (int r=0; r<Revs; r++)
				{
					if ( 
						(!ok) &&
						(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used) &&
						( (Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok) ||
						(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok) ) )
					{
						memcpy(Disk.Cyls[c].Heads[h].Sectors[s].Merged.Data, Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, SectSize);
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.used = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc1ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc2ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok;
						ok=true;
					}
				}
				for (int r=0; r<Revs; r++)
				{
					if ( 
						(!ok) &&
						(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used) )
					{
						memcpy(Disk.Cyls[c].Heads[h].Sectors[s].Merged.Data, Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, SectSize);
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.used = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc1ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc2ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok;
						ok=true;
					}
				}
				if (!ok)
				{
					printf("# MISSING: No usable copy of sector %02d/%02d/%02d found.\n", c,h,s);
				}
			}
		}
	}
	printf("# Merge done.\n");
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
				write(fd, Disk.Cyls[c].Heads[h].Sectors[s].Merged.Data, SectSize);
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
