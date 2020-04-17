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
#include "Buffer.h"

///////////////////////////////////////////////////////////

VirtualDisk::VirtualDisk()
{
	FinalDisk=NULL;
	Cyls=0;
	Heads=0;
	Sects=0;
	Revs=0;
	SectSize=0;
}

VirtualDisk::~VirtualDisk()
{
	for (int i=0; i<Cyls; i++)
	{
		for (int j=0; j<Heads; j++)
		{
			for (int k=0; k<Sects; k++)
			{
				for (int l=0; l<Revs; l++)
				{
					delete[](Disk.Cyls[i].Heads[j].Sectors[k].Revs[l].Data);
				}
				delete[](Disk.Cyls[i].Heads[j].Sectors[k].Revs);
			}
			delete[](Disk.Cyls[i].Heads[j].Sectors);
		}
		delete[](Disk.Cyls[i].Heads);
	}
	delete[](Disk.Cyls);

	delete(FinalDisk);
}

void VirtualDisk::SetLayout(u8 c, u8 h, u8 s, u8 r, u16 ss)
{ 
	Cyls=c; 
	Heads=h; 
	Sects=s;
	Revs=r;
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
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.Data = NULL;
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.used=false;
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.crc1ok=false;
				Disk.Cyls[i].Heads[j].Sectors[k].Merged.crc2ok=false;
			}
		}
	}
}

void VirtualDisk::AddSector(u8 c, u8 h, u8 s, u8 r, void *p, u32 size, bool crc1ok, bool crc2ok)
{
	clog(1,"### Cyl %02d # Head %01d # Sect %02d\r", c,h,s);
	memcpy(Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, p, min(size,SectSize));
	Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used=true;
	Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok=crc1ok;
	Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok=crc2ok;
}

void *VirtualDisk::GetSector(u8 c, u8 h, u8 s)
{
	if (FinalDisk==NULL)
	{
		clog(0, "ERR: VirtualDisk has not been merged yet. This is a code bug.\n");
		return NULL;
	}
	return FinalDisk->GetBuffer()+(((c*Heads+h)*Sects+s)*SectSize);
}

void *VirtualDisk::GetSector(u16 blk)
{
	if (FinalDisk==NULL)
	{
		clog(0, "ERR: VirtualDisk has not been merged yet. This is a code bug.\n");
		return NULL;
	}
	return FinalDisk->GetBuffer()+blk*SectSize;
}

void VirtualDisk::MergeRevs()
{
	clog(1, "# Merging sector copies.\n");

	if (FinalDisk!=NULL)
	{
		clog(0, "ERR: VirtualDisk was already merged. This is a code bug.\n");
		return;
	}
	FinalDisk = new Buffer();
	FinalDisk->Alloc(Cyls*Heads*Sects*SectSize);

	u32 bad_count=0;
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
						memcpy(FinalDisk->GetBuffer()+(((c*Heads+h)*Sects+s)*SectSize), Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, SectSize);
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
						memcpy(FinalDisk->GetBuffer()+(((c*Heads+h)*Sects+s)*SectSize), Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, SectSize);
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
						memcpy(FinalDisk->GetBuffer()+(((c*Heads+h)*Sects+s)*SectSize), Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].Data, SectSize);
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.used = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].used;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc1ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc1ok;
						Disk.Cyls[c].Heads[h].Sectors[s].Merged.crc2ok = Disk.Cyls[c].Heads[h].Sectors[s].Revs[r].crc2ok;
						ok=true;
					}
				}
				if (!ok)
				{
					clog(2,"# MISSING: No usable copy of sector %02d/%02d/%02d found.\n", c,h,s);
					bad_count++;
				}
			}
		}
	}
	clog(1,"# Final Disk has %d/%d missing or bad sectors! That's %.1f%% of the disk damaged.\n", bad_count, Cyls*Heads*Sects, (float)(100*bad_count)/(float)(Cyls*Heads*Sects));
	clog(1,"# Merge done.\n");
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
