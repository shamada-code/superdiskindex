///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "BitStream.h"
#include "FormatDiskIBM.h"
#include "Buffer.h"
#include "MFM.h"
#include "Helpers.h"
#include "CRC.h"
#include "VirtualDisk.h"

///////////////////////////////////////////////////////////

#pragma pack(1)

struct bpb_dos30 {
    u16 byte_per_sect;
    u8 sect_per_cluster;
    u16 reserved_sectors;
    u8 fat_count;
    u16 root_entries;
    u16 sector_count;
    u8 media_descriptor;
    u16 sectors_per_fat;
    u16 sectors_per_track;
    u16 head_count;
};

struct bootsect_fat12 {
    u8 jump_instr[3];
    u8 oem_name[8];
    bpb_dos30 bpb;
	u8 bootcode[512-28-2];
	u16 signature;
};

struct bootsect_st {
    u16 jump_instr;
    u8 oem_name[6];
    u8 serial[3];
    bpb_dos30 bpb;
};

struct dir_entry {
    char name[8];
    char ext[3];
    u8 attrs;
    u8 unk1;
    u8 create_time;
    u16 create_time2;
    u16 create_date;
    u16 access_time;
    u16 perm;
    u16 mod_time;
    u16 mod_date;
    u16 first_cluster;
    u32 size;
};

enum direntry_attr {
    DEA_RO = 0x01,
    DEA_HID = 0x02,
    DEA_SYS = 0x04,
    DEA_VOL = 0x08,
    DEA_DIR = 0x10,
    DEA_ARC = 0x20,
    DEA_DEV = 0x40,
    DEA_RES = 0x80,
};

#pragma pack(0)

///////////////////////////////////////////////////////////

char const *FormatDiskIBM::GetName() { return "IBM/PC/Atari"; }

u8 FormatDiskIBM::GetSyncWordCount()
{
	return 2;
}

u32 FormatDiskIBM::GetSyncWord(int n)
{
	switch (n)
	{
		case 0:
			return 0x44895554;
		case 1:
			return 0x44895545;
	}
	return 0;
}

u32 FormatDiskIBM::GetSyncBlockLen(int n)
{
	switch (n)
	{
		case 0:
			return (4+2)*2;
		case 1:
			return (512+2)*2;
	}
	return 0;
}

///////////////////////////////////////////////////////////

// bool FormatDiskIBM::Detect()
// {
// 	Buffer *MFMBuffer = Bits->GetBuffer();
// 	u16 *MFMData = (u16 *)(MFMBuffer->GetBuffer());

// 	for (int i=0; i<8; i++)
// 	{
// 		u8 val = mfm_decode(swap16(MFMData[i]));
// 		printf("# DATA: %02x\n", val);
// 	}

// 	return false;
// }

void FormatDiskIBM::HandleBlock(Buffer *buffer, int currev)
{
	//printf("Handling Block with size %d.\n", buffer->GetFill());
	buffer->MFMDecode();
	u8 *db = buffer->GetBuffer();
	hexdump(db, 64);
	if ( (db[0]==0xa1) && (db[1]==0xfe) )
	{
		CRC16_ccitt crc(~0);
		crc.Feed(0xa1a1, true);
		crc.Block(db, 4, true);

		clog(2,"# Sector Header + %02d/%01d/%02d + crc %04x (%s)\n", db[2], db[3], db[4], (db[6]<<8)|db[7], crc.Check()?"OK":"BAD");
		if (crc.Check())
		{
			LastCyl=max(LastCyl,db[2]);
			LastHead=max(LastHead,db[3]);
			LastSect=max(LastSect,db[4]-1); // Sectors on ibm are starting with "1"!
			cur_c=db[2];
			cur_h=db[3];
			cur_s=db[4];
		}
	}
	if ( (db[0]==0xa1) && (db[1]==0xfb) )
	{
		CRC16_ccitt crc(~0);
		crc.Feed(0xa1a1, true);
		crc.Block(db, 1+256+1, true);

		clog(2,"# Sector Data + %04x (%s)\n", (db[2+512+0]<<8)|db[2+512+1], crc.Check()?"OK":"BAD");
		if ((cur_c<0)||(cur_h<0)||(cur_s<0))
		{
			clog(1, "# WARN: Found sector data block without valid sector header!\n");
		} else {
			// process
			if (Config.verbose>=3) hexdump(db+2, buffer->GetFill()-4);

			if (Disk!=NULL)
			{
				Disk->AddSector(cur_c, cur_h, cur_s>0?cur_s-1:0, currev, buffer->GetBuffer()+2, buffer->GetFill()-4, true, crc.Check());
			}

			cur_c=cur_h=cur_s=-1;
		}
	}
	//hexdump(buffer->GetBuffer(), min(buffer->GetFill(), 64));
}

bool FormatDiskIBM::Analyze()
{
	// boot block
	bootsect_fat12 *boot0 = (bootsect_fat12 *)(Disk->GetSector(0));
	clog(2, "# BOOT: Signature = $%04x\n",boot0->signature);
	clog(2, "# BPB: byte_per_sect = %d\n",boot0->bpb.byte_per_sect);
	clog(2, "# BPB: sect_per_cluster = %d\n",boot0->bpb.sect_per_cluster);
	clog(2, "# BPB: reserved_sectors = %d\n",boot0->bpb.reserved_sectors);
	clog(2, "# BPB: fat_count = %d\n",boot0->bpb.fat_count);
	clog(2, "# BPB: root_entries = %d\n",boot0->bpb.root_entries);
	clog(2, "# BPB: sector_count = %d\n",boot0->bpb.sector_count);
	clog(2, "# BPB: media_descriptor = $%02x\n",boot0->bpb.media_descriptor);
	clog(2, "# BPB: sectors_per_fat = %d\n",boot0->bpb.sectors_per_fat);
	clog(2, "# BPB: sectors_per_track = %d\n",boot0->bpb.sectors_per_track);
	clog(2, "# BPB: head_count = %d\n",boot0->bpb.head_count);
	if (boot0->signature!=0xaa55) { clog(1, "# BOOT: Signature mismatch (%04x!=%04x).\n", boot0->signature, 0xaa55); return false; }
	clog(1, "# BOOT: Signature OK.\n");
	if (boot0->bpb.byte_per_sect!=512) { clog(1, "# BOOT: Not the expected sector size (%d!=%d).\n", boot0->bpb.byte_per_sect, 512); return false; }
	clog(1, "# BOOT: Sector Size OK.\n");

	//root folder
	int rootblk = boot0->bpb.reserved_sectors+boot0->bpb.fat_count*boot0->bpb.sectors_per_fat;
	int root_sects = boot0->bpb.root_entries*sizeof(dir_entry)/boot0->bpb.byte_per_sect; 
	ParseDirectory(0, rootblk, root_sects, "/");
	return false;
}

void FormatDiskIBM::ParseDirectory(int fd, u32 block, u32 blkcount, char const *prefix)
{
	bootsect_fat12 *boot0 = (bootsect_fat12 *)(Disk->GetSector(0));
	int entries_per_sect = boot0->bpb.byte_per_sect/sizeof(dir_entry);
	for (int rs=0; rs<blkcount; rs++)
	{
		dir_entry *rootdir = (dir_entry *)(Disk->GetSector(block+rs));
		for (int i=0; i<entries_per_sect; i++)
		{
			if (rootdir[i].attrs==0xf)
			{
				//clog(1, "# --lfn entry--\n");
			} else {
				if ((rootdir[i].name[0]>0x00) && !((rootdir[i].name[0]==0xe5)&&(rootdir[i].first_cluster==0)))
				{
					char sbuf0[8+1]; memcpy(sbuf0, rootdir[i].name, 8); sbuf0[8]=0; for (int si=7; si>=0; si--) if (sbuf0[si]==0x20) sbuf0[si]=0;
					char sbuf1[3+1]; memcpy(sbuf1, rootdir[i].ext, 3); sbuf1[3]=0; for (int si=2; si>=0; si--) if (sbuf1[si]==0x20) sbuf1[si]=0;
					char sbuf[4096]; sprintf(sbuf, "%s%s%s%s%s", prefix, sbuf0, sbuf1[0]!=0?".":"", sbuf1, rootdir[i].attrs==DEA_DIR?"/":"");
					clog(1, "%-60s %8d <%c%c%c%c%c%c> %s\n", 
						sbuf,
						rootdir[i].size, 
						rootdir[i].attrs==DEA_VOL?'V':'.',
						rootdir[i].attrs==DEA_DIR?'D':'.',
						rootdir[i].attrs==DEA_RO?'R':'.',
						rootdir[i].attrs==DEA_SYS?'S':'.',
						rootdir[i].attrs==DEA_ARC?'A':'.',
						rootdir[i].attrs==DEA_HID?'H':'.',
						rootdir[i].name[0]==0xe5?"<DELETED>":""
						);
					if (rootdir[i].attrs==DEA_DIR)
					{
						// disabled because we're missing a cluster->sector mapping
						//ParseDirectory(fd, rootdir[i].first_cluster, 1, sbuf);
					}
				}
			}
		}
	}
}