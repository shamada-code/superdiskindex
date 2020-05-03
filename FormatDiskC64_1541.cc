///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Global.h"
#include "Helpers.h"
#include "CRC.h"
#include "VirtualDisk.h"
#include "DiskMap.h"
#include "DiskLayout.h"
#include "FormatDiskC64_1541.h"
#include "CBM.h"

///////////////////////////////////////////////////////////

struct _1540_sysblk
{
	u8 dir0_track;
	u8 dir0_sect;
	u8 format;
	u8 unused;
	u32 bam[35];
	char label[18];
	u8 id1;
	u8 id2;
	u8 space;
	u8 dosversion;
	u8 dosformat;
	// rest of sector is unused
};

struct _1540_direntry
{
	u8 next_track; // only valid for first entry
	u8 next_sect; // only valid for first entry
	u8 type;
	u8 data_track;
	u8 data_sect;
	char filename[16];
	u8 rel0;
	u8 rel1;
	u8 rel2;
	u8 unused[4];
	u8 ovr_track;
	u8 ovr_sect;
	u16 size_in_blocks; // little endian
};

struct _1540_dirblk
{
	_1540_direntry entries[8];
};

///////////////////////////////////////////////////////////

FormatDiskC64_1541::FormatDiskC64_1541() : cur_c(-1),cur_h(-1),cur_s(-1),LayoutLocked(false) 
{ 
	SectSize=256; 
	DLayout = new DiskLayout();
	// set layout to expected 1541 format
	DLayout->SetLayout(35,1,0,true);
	int i=0;
	while (i<17) { DLayout->SetTrackLen(i++, 21); }
	while (i<24) { DLayout->SetTrackLen(i++, 19); }
	while (i<30) { DLayout->SetTrackLen(i++, 18); }
	while (i<35) { DLayout->SetTrackLen(i++, 17); }
}

FormatDiskC64_1541::~FormatDiskC64_1541() 
{

}

char const *FormatDiskC64_1541::GetName()
{
	return "C64/1541";
}

u8 FormatDiskC64_1541::GetSyncWordCount()
{
	return 2;
}

u32 FormatDiskC64_1541::GetSyncWord(int n)
{
	switch (n)
	{
		case 0: return 0xfffffd49;
		case 1: return 0xfffffd57;
	}
	return 0;
}

u32 FormatDiskC64_1541::GetSyncBlockLen(int n)
{
	switch (n)
	{
		case 0: return 10;
		case 1: return 325;
	}
	return 0;
}

u16 FormatDiskC64_1541::GetMaxExpectedCylinder() { return 42; }
u16 FormatDiskC64_1541::GetMaxExpectedSector() { return 25; }

void FormatDiskC64_1541::PreTrackInit()
{
	cur_c=cur_h=cur_s=-1;
}

void FormatDiskC64_1541::HandleBlock(Buffer *buffer, int currev)
{
	buffer->GCRDecode(2,6);
	u8 *db = buffer->GetBuffer();
	if (*db==0x8)
	{
		//hexdump(db, 7);

		CRC8_xor crc(0);
		//crc.Feed(0xa1a1, true);
		crc.Block(db+1, 5, false);

		clog(2,"# Sector Header + %02d/%02d + crc %02x (%s)\n", db[3], db[2], db[1], crc.Check()?"OK":"BAD");
		if ( 
			(crc.Check()) && 
			(db[3]<=GetMaxExpectedCylinder()) && // only allow tracks in expected range
			(db[2]<=GetMaxExpectedSector()) && // only allow sectors in expected range
			1)
		{
			if (!LayoutLocked)
			{
				LastCyl=maxval(LastCyl,db[3]-1);  // Tracks on cbm are starting with "1"!
				LastSect=maxval(LastSect,db[2]);
			}
			cur_c=db[3]-1;
			cur_h=0;
			cur_s=db[2];
		}
	}	
	if (*db==0x7)
	{
		//hexdump(db, 256);

		CRC8_xor crc(0);
		crc.Block(db+1, 257, false);
		
		clog(2,"# Sector Data + %04x (%s)\n", db[1+256], crc.Check()?"OK":"BAD");
		if ((cur_c<0)||(cur_h<0)||(cur_s<0))
		{
			clog(2, "# WARN: Found sector data block without valid sector header!\n");
		} else {
			// process
			if (Disk!=NULL)
			{
				Disk->AddSector(cur_c, cur_h, cur_s, currev, buffer->GetBuffer()+1, minval(buffer->GetFill()-1, 256), true, crc.Check());
			}

			cur_c=cur_h=cur_s=-1;
		}
	}
}

bool FormatDiskC64_1541::Analyze()
{
	char sbuf[64];

	if (Disk->GetLayoutCylinders()!=35) return false; // only standard c64 floppy format is supported for now.
	if (Disk->GetLayoutHeads()!=1) return false; // only standard c64 floppy format is supported for now.
	if (Disk->GetLayoutSectors()!=21) return false; // only standard c64 floppy format is supported for now.

	// boot block
	_1540_sysblk *sys = (_1540_sysblk *)(Disk->GetSector(17,0,0));
	clog(1, "# DISK: Format = %s\n",sys->format=='A'?"1540":"Unknown");
	//if (sys->format!='A') return false; // only standard c64 floppy format is supported for now.
	CBM::petscii2ascii(sbuf, sys->label, 18);
	clog(1, "# DISK: Label = %s\n",sbuf);
	clog(1, "# DISK: ID = $%02x $%02x (%c%c)\n",sys->id1,sys->id2,sys->id1,sys->id2);
	clog(1, "# DISK: DOSVersion = %c%c\n",sys->dosversion,sys->dosformat);

	// prep and init dmap
	{
		DMap = new DiskMap(DLayout->GetTotalSectors());
		DMap->SetBitsSector(DLayout->CHStoBLK(17,0,0), DMF_ROOTBLOCK);
		// set crc states
		for (u32 i=0; i<DLayout->GetTotalSectors(); i++)
		{
			u16 c,h,s;
			DLayout->BLKtoCHS(i, &c,&h,&s);
			if (Disk->IsSectorCRCBad(c,h,s)) DMap->SetBitsSector(i, DMF_CRC_LOWLEVEL_BAD);
			if (Disk->IsSectorMissing(c,h,s)) DMap->SetBitsSector(i, DMF_MISSING);
		}
		// scan/mirror blockmap
		for (int i=0; i<35; i++)
		{
			u8 freesect = sys->bam[i]&0xff;
			u32 mapbits = sys->bam[i]>>8;
			// count set bits
			u8 setbits = 0;
			for (int j=0; j<24; j++) setbits+=((mapbits>>j)&1);
			if (freesect!=setbits) clog(2, "# BAM: Track %d has %d blocks marked free, but reports %d free blocks.\n", i, setbits, freesect);
			for (int j=0; j<DLayout->GetTrackLen(i); j++)
			{
				DMap->SetBitsSector(DLayout->CHStoBLK(i,0, j), (mapbits&(0x1<<j))>0?0:DMF_BLOCK_USED);
			}
		}
	}

	// dir block
	u8 dir_trk = 17;//sys->dir0_track-1;
	u8 dir_sect = 1;//sys->dir0_sect;

	clog(1, "#############################################################\n");

	// Listing
	{
		int fd=-1;
		char *fnout=NULL;
		if (Config.gen_listing)
		{
			gen_output_filename(&fnout, Config.fn_out, OT_LISTING, ".lst", OutputParams("c64", 0,0,0,0));
			clog(1,"# Generating file listing '%s'.\n",fnout);
			fd = open(fnout, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);
		}
		//if ((Config.gen_listing)&&(fd>=0))
		{
			memset(sbuf, 0, sizeof(sbuf));
			CBM::petscii2ascii(sbuf, sys->label, 18);
			cdprintf(Config.show_listing, Config.gen_listing, fd, "0 \"%-16s\" %c%c %c%c\n", sbuf, sys->id1, sys->id2, sys->dosversion, sys->dosformat);
		}
		while ((dir_trk!=0)&&(dir_trk!=0xff))
		{
			DMap->SetBitsSector(DLayout->CHStoBLK(dir_trk,0,dir_sect), DMF_DIRECTORY);
			_1540_dirblk *dir = (_1540_dirblk *)(Disk->GetSector(dir_trk,0,dir_sect));
			if (dir==NULL) break;
			for (int i=0; i<8; i++)
			{
				if (dir->entries[i].type==0) continue;

				clog(2,"$%d.%d", dir->entries[i].data_track, dir->entries[i].data_sect);
				char const *fstatestr = "";
				int fstate=0;
				if (dir->entries[i].data_track>0)
				{
					fstate = ScanFile(dir->entries[i].data_track-1, dir->entries[i].data_sect, dir->entries[i].size_in_blocks);
				}

				//if ((Config.gen_listing)&&(fd>=0))
				{
					CBM::petscii2ascii(sbuf, dir->entries[i].filename, 16);
					char const *etype="???";
					switch (dir->entries[i].type)
					{
						case 0x80: etype="DEL"; break;
						case 0x81: etype="SEQ"; break;
						case 0x82: etype="PRG"; break;
						case 0x83: etype="USR"; break;
						case 0x84: etype="REL"; break;
						case 0: etype="NUL"; break;
						default: etype="???";
					}
					switch (fstate)
					{
						case -1: fstatestr = "<BADSIZE>"; break;
						case -2: fstatestr = "<BADLNKS>"; break;
						case -3: fstatestr = "<BADPTR>"; break;
						case -4: fstatestr = "<BADCRC>"; break;
					}
					cdprintf(Config.show_listing, Config.gen_listing, fd, "%-4d \"%-16s\" %s   %s\n", dir->entries[i].size_in_blocks, sbuf, etype, fstatestr);
				}
			}
			dir_trk = dir->entries[0].next_track-1;
			dir_sect = dir->entries[0].next_sect;
		}
		if ((Config.gen_listing)&&(fd>=0))
		{
			close(fd);
			free(fnout);
		}
	}

	// Output DiskMaps
	{
		//if (Config.gen_maps)
		{
			char *fnout=NULL;
			gen_output_filename(&fnout, Config.fn_out, OT_MAPS, ".maps", OutputParams("c64", 0,0,0,0));
			if (Config.gen_maps)
			{
				clog(1,"# Generating block/usage/healthmaps '%s'.\n",fnout);
			}
			if (DMap)
			{
				DMap->OutputMaps(fnout);
			}
			free(fnout);
		}
	}

	// Export
	if (Config.gen_export)
	{
		char *fnout=NULL;
		gen_output_filename(&fnout, Config.fn_out, OT_DISKIMAGE, ".d64", OutputParams("c64", 0,0,0,0));
		clog(1,"# Generating diskimage '%s'.\n",fnout);
		if (Disk)
		{
			Disk->ExportD64(fnout);
		}
		free(fnout);
	}

	return true;
}

char const *FormatDiskC64_1541::GetDiskTypeString()
{
	return "C=64";
}

char const *FormatDiskC64_1541::GetDiskSubTypeString()
{
	return "DD/1540";
}

int FormatDiskC64_1541::ScanFile(u8 trk, u8 sect, int dir_blkcount, int cur_blkcount)
{
	if (DMap->GetSector(DLayout->CHStoBLK(trk,0,sect))&DMF_DATA) {
		clog(2, "# File chain link loop or cross link detected. skipping further travel.\n");
		return -2;
	}
	DMap->SetBitsSector(DLayout->CHStoBLK(trk,0,sect),DMF_DATA);
	u8 *sdata = (u8 *)Disk->GetSector(trk, 0, sect);
	if (sdata==NULL) { clog(2, " <T/S Pointer beyond disk limits>\n"); return -3; }
	clog(2, " -> $%d.%d", sdata[0], sdata[1]);
	if (sdata[0]>0) return ScanFile(sdata[0]-1, sdata[1], dir_blkcount, cur_blkcount+1);
	//else 
	clog(2, " (chain %d blocks, dir %d blocks)\n", cur_blkcount+1, dir_blkcount);
	return ((cur_blkcount+1)==dir_blkcount)?0:-1;
}
