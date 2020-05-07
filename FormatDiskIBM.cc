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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "DiskMap.h"
#include "DiskLayout.h"

///////////////////////////////////////////////////////////

enum DiskTypes
{
	DT_UNKNOWN = 0,
	DT_IBMPC,
	DT_ATARIST,
};

enum DiskSubTypes
{
	DST_UNKNOWN = 0,
	DST_3_5DD,
	DST_3_5HD,
	DST_5_25DD,
	DST_5_25HD,
};

///////////////////////////////////////////////////////////

#pragma pack(1)

struct bpb_base {
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

		u32 hidden_sectors;
		u32 sector_count32;
};

struct bpb_fat12 {
	u8 drive_num;
	u8 reserved1;
	u8 ext_boot_sig;
	u32 volume_id;
	char volume_label[11];
	char fat_type[8];
	u8 bootcode[512-62-2];
};

struct bpb_fat32 {
	u32 sectors_per_fat32;
	u16 extflags;
	u16 fsver;
	u32 root_cluster;
	u16 fsinfo_sector;
	u16 boot_backup_sector;
	u8 drive_num;
	u8 reserved1;
	u8 ext_boot_sig;
	u32 volume_id;
	char volume_label[11];
	char fat_type[8];
	u8 bootcode[512-90-2];
};

struct bootsect_ibm {
    u8 jump_instr[3];
    u8 oem_name[8];
    bpb_base bpb;
		union {
    	bpb_fat12 bpb12;
			bpb_fat32 bpb32;
		} bpbext;
	u16 signature;
};

struct bootsect_st {
    u16 jump_instr;
    u8 oem_name[6];
    u8 serial[3];
    bpb_base bpb;
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

FormatDiskIBM::FormatDiskIBM() : cur_c(-1),cur_h(-1),cur_s(-1)
{
	InitLayout();
}

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

void FormatDiskIBM::PreTrackInit()
{
	cur_c=cur_h=cur_s=-1;
}

void FormatDiskIBM::HandleBlock(Buffer *buffer, int currev)
{
	//printf("Handling Block with size %d.\n", buffer->GetFill());
	buffer->MFMDecode();
	u8 *db = buffer->GetBuffer();

	// Sector header
	if ( (db[0]==0xa1) && (db[1]==0xfe) )
	{
		CRC16_ccitt crc(~0);
		crc.Feed(0xa1a1, true);
		crc.Block(db, 4, true);

		clog(2,"# Sector Header + %02d/%01d/%02d + crc %04x (%s)\n", db[2], db[3], db[4], (db[6]<<8)|db[7], crc.Check()?"OK":"BAD");
		if (crc.Check()) {
			if (DLayout->FoundSector(db[2],db[3],db[4]-1))
			{
				cur_c=db[2];
				cur_h=db[3];
				cur_s=db[4]-1;
			}
		}
	}

	// Sector data
	if ( (db[0]==0xa1) && (db[1]==0xfb) )
	{
		CRC16_ccitt crc(~0);
		crc.Feed(0xa1a1, true);
		crc.Block(db, 1+256+1, true);

		clog(2,"# Sector Data + %04x (%s)\n", (db[2+512+0]<<8)|db[2+512+1], crc.Check()?"OK":"BAD");
		if ((cur_c<0)||(cur_h<0)||(cur_s<0))
		{
			clog(2, "# WARN: Found sector data block without valid sector header!\n");
		} else {
			// process
			if (Config.verbose>=3) hexdump(db+2, buffer->GetFill()-4);

			if (Disk==NULL)
			{
				// first pass
				if ((cur_c==0)&&(cur_h==0)&&(cur_s==0))
				{
					// we found the boot block
					// use info in bpb to setup disk layout
					bootsect_ibm *boot0 = (bootsect_ibm *)(db+2);
					// sanity check boot block
					if (boot0->bpb.byte_per_sect==512)
					{
						DLayout->FoundSector(
							(boot0->bpb.sector_count/(boot0->bpb.head_count*boot0->bpb.sectors_per_track))-1,
							boot0->bpb.head_count-1,
							boot0->bpb.sectors_per_track-1
							);
						DLayout->LockLayout();
					}
				}
			}

			if (Disk!=NULL)
			{
				Disk->AddSector(cur_c, cur_h, cur_s, currev, buffer->GetBuffer()+2, buffer->GetFill()-4, true, crc.Check());
			}

			cur_c=cur_h=cur_s=-1;
		}
	}
	//hexdump(buffer->GetBuffer(), minval(buffer->GetFill(), 64));
}

bool FormatDiskIBM::Analyze()
{
	// boot block
	bootsect_ibm *boot0 = (bootsect_ibm *)(Disk->GetSector(0));
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
	if (boot0->signature!=0xaa55) { clog(1, "# BOOT: Signature mismatch (%04x!=%04x).\n", boot0->signature, 0xaa55); /*return false;*/ }
	clog(1, "# BOOT: Signature OK.\n");
	if (boot0->bpb.byte_per_sect!=512) { clog(1, "# BOOT: Not the expected sector size (%d!=%d).\n", boot0->bpb.byte_per_sect, 512); return false; }
	clog(1, "# BOOT: Sector Size OK.\n");

	// This seems to be a proper ibm formatted disk.
	SetDiskType(DT_IBMPC);
	if (Disk->GetLayoutCylinders()==40)
	{
		if (Disk->GetLayoutSectors()==9) SetDiskSubType(DST_5_25DD);
	}
	if (Disk->GetLayoutCylinders()==80)
	{
		if (Disk->GetLayoutSectors()==15) SetDiskSubType(DST_5_25HD);
		if (Disk->GetLayoutSectors()==9) SetDiskSubType(DST_3_5DD);
		if (Disk->GetLayoutSectors()==18) SetDiskSubType(DST_3_5HD);
	}

	if (boot0->signature!=0xaa55)
	{
		// probably not a pc, but an atari st disk.
		SetDiskType(DT_ATARIST);

		u16 wsum=0;
		u16 *p16 = (u16 *)boot0;
		for (int i=0; i<256; i++) wsum+=swap(p16[i]);
		if (wsum==0x1234)
		{
			clog(1, "# BOOT: This is a bootable Atari ST disk.\n");
		}
	}

	// Detect FAT settings
	DetectFAT();
	int fatsects = boot0->bpb.sectors_per_fat>0?boot0->bpb.sectors_per_fat:boot0->bpbext.bpb32.sectors_per_fat32;
	clog(1, "# FAT settings: Type=%s, Sectors=%d, Copies=%d\n", FatTypeName(FatType), fatsects, boot0->bpb.fat_count);

	// Initialize DiskMap
	{
		DMap = new DiskMap(Disk->GetSectorCount(), clustersize(), cluster2sector(0));
		DMap->SetBitsSector(0, DMF_BOOTBLOCK);
		for (int i=0; i<boot0->bpb.fat_count; i++) {
			for (int j=0; j<fatsects; j++) {
				DMap->SetBitsSector(i*fatsects+j+boot0->bpb.reserved_sectors, DMF_BLOCKMAP);
			}
		}
		
		int rootblk = boot0->bpb.reserved_sectors+fatsects*boot0->bpb.fat_count;
		int root_sects = boot0->bpb.root_entries*sizeof(dir_entry)/boot0->bpb.byte_per_sect; 
		for (int i=0; i<root_sects; i++)
		{
			DMap->SetBitsSector(rootblk+i, DMF_ROOTBLOCK);
		}
		for (u32 i=0; i<Disk->GetSectorCount(); i++)
		{
			if (Disk->IsSectorMissing(i)) DMap->SetBitsSector(i, DMF_MISSING);
			if (Disk->IsSectorCRCBad(i)) DMap->SetBitsSector(i, DMF_CRC_LOWLEVEL_BAD);
		}
	}

	// scan fat and mirror content in DiskMap
	{
		for (int i=0; i<boot0->bpb.fat_count; i++) {
			int j=0;
			u32 s = i*fatsects+j+boot0->bpb.reserved_sectors;
			u8 *fat = (u8 *)Disk->GetSector(s);
			
			if (FatType==FAT12)
			{
				fat+=3;
				for (u32 k=1; k<ClusterCount/2; k++) {
					u16 a = fat[0] | ((fat[1]&0xf)<<8);
					u16 b = ((fat[1]&0xf0)>>4) | (fat[2]<<4);
					if (0) {}
					else if ((a>=0x2) && (a<0xff0)) DMap->SetBitsCluster(k*2+0, DMF_BLOCK_USED);
					else if ((a>=0xff8) && (a<0x1000)) DMap->SetBitsCluster(k*2+0, DMF_BLOCK_USED);
					else if (a==0xff7) DMap->SetBitsCluster(k*2+0, DMF_BLOCK_BAD);
					if (0) {}
					else if ((b>=0x2) && (b<0xff0)) DMap->SetBitsCluster(k*2+1, DMF_BLOCK_USED);
					else if ((b>=0xff8) && (b<0x1000)) DMap->SetBitsCluster(k*2+1, DMF_BLOCK_USED);
					else if (b==0xff7) DMap->SetBitsCluster(k*2+1, DMF_BLOCK_BAD);
					fat+=3;
				}
			} else if (FatType==FAT16) {
				//:todo: add code for scanning fat16
			} else if (FatType==FAT32) {
				//:todo: add code for scanning fat32
			}
		}
	}

	u32 bad_sectors_in_used_blocks=0;
	// Listing
	{
		int fd=-1;
		char *fnout=NULL;
		if (Config.gen_listing)
		{
			gen_output_filename(&fnout, Config.fn_out, OT_LISTING, ".lst", OutputParams(DiskType==DT_ATARIST?"atari-st":"pc", 0,0,0,0));
			clog(1,"# Generating file listing '%s'.\n",fnout);

			fd = open(fnout, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);
		}

		int rootblk = boot0->bpb.reserved_sectors+boot0->bpb.fat_count*boot0->bpb.sectors_per_fat;
		int root_sects = boot0->bpb.root_entries*sizeof(dir_entry)/boot0->bpb.byte_per_sect; 

		//if ((Config.gen_listing)&&(fd>=0))
		{
			cdprintf(Config.show_listing, Config.gen_listing, fd, "Volume: %s\n", "N/A");
			cdprintf(Config.show_listing, Config.gen_listing, fd, "\n");
			cdprintf(Config.show_listing, Config.gen_listing, fd, "%-60s     Size  Flags\n", "Filename");
			cdprintf(Config.show_listing, Config.gen_listing, fd, "-----------------------------------------------------------------------------------------\n");
		}
		ParseDirectory(fd, rootblk, root_sects, "/", &bad_sectors_in_used_blocks);
		if ((Config.gen_listing)&&(fd>=0))
		{
			close(fd);
			free(fnout);
		}
		clog(1, "# Found %d bad/missing sectors in files/directories.\n", bad_sectors_in_used_blocks);
		clog(1, "# Found %d bad/missing sectors in unused space.\n", (Disk->GetMissingCount()+Disk->GetCRCBadCount())-bad_sectors_in_used_blocks);
	}

	// Output DiskMaps
	{
		//if (Config.gen_maps)
		{
			char *fnout=NULL;
			gen_output_filename(&fnout, Config.fn_out, OT_MAPS, ".maps", OutputParams(DiskType==DT_ATARIST?"atari-st":"pc", 0,0,0,0));
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
		gen_output_filename(&fnout, Config.fn_out, OT_DISKIMAGE, DiskType==DT_ATARIST?".st":".img", OutputParams(DiskType==DT_ATARIST?"atari-st":"pc", 0,0,0,0));
		clog(1,"# Generating diskimage '%s'.\n",fnout);
		if (Disk)
		{
			Disk->ExportIMG(fnout);
		}
		free(fnout);
	}

	return true;
}

void FormatDiskIBM::ParseDirectory(int fd, u32 block, u32 blkcount, char const *prefix, u32 *bad_sector_count)
{
	bootsect_ibm *boot0 = (bootsect_ibm *)(Disk->GetSector(0));
	int entries_per_sect = boot0->bpb.byte_per_sect/sizeof(dir_entry);
	for (u32 rs=0; rs<blkcount; rs++)
	{
		dir_entry *rootdir = (dir_entry *)(Disk->GetSector(block+rs));
		if (rootdir==NULL) continue;
		for (int i=0; i<entries_per_sect; i++)
		{
			if (rootdir[i].attrs==0xf)
			{
				//clog(1, "# --lfn entry--\n");
			} else {
				u8 etype = (u8)(rootdir[i].name[0]);
				if ((etype>0x00) && !((etype==0xe5)&&(rootdir[i].first_cluster==0)))
				{
					char sbuf0[8+1]; memcpy(sbuf0, rootdir[i].name, 8); sbuf0[8]=0; for (int si=7; si>=0; si--) if (sbuf0[si]==0x20) sbuf0[si]=0;
					char sbuf1[3+1]; memcpy(sbuf1, rootdir[i].ext, 3); sbuf1[3]=0; for (int si=2; si>=0; si--) if (sbuf1[si]==0x20) sbuf1[si]=0;
					char sbuf[4096]; sprintf(sbuf, "%s%s%s%s%s", prefix, sbuf0, sbuf1[0]!=0?".":"", sbuf1, rootdir[i].attrs==DEA_DIR?"/":"");
					//if ((Config.gen_listing)&&(fd>=0))
					{
						cdprintf(Config.show_listing, Config.gen_listing, fd, "%-60s %8d  <%c%c%c%c%c%c> %s\n", 
							sbuf,
							rootdir[i].size, 
							(rootdir[i].attrs&DEA_VOL)?'V':'.',
							(rootdir[i].attrs&DEA_DIR)?'D':'.',
							(rootdir[i].attrs&DEA_RO)?'R':'.',
							(rootdir[i].attrs&DEA_SYS)?'S':'.',
							(rootdir[i].attrs&DEA_ARC)?'A':'.',
							(rootdir[i].attrs&DEA_HID)?'H':'.',
							etype==0xe5?"<DELETED>":""
							);
					}
					if (
						(rootdir[i].attrs&DEA_DIR) &&
						(memcmp(rootdir[i].name, ".       ",8)!=0) &&
						(memcmp(rootdir[i].name, "..      ",8)!=0) )
					{
						DMap->SetBitsCluster(rootdir[i].first_cluster, DMF_DIRECTORY);
						ParseDirectory(fd, cluster2sector(rootdir[i].first_cluster), 1, sbuf, bad_sector_count);
					} else if (((rootdir[i].attrs&DEA_VOL)==0)&&((rootdir[i].attrs&DEA_DIR)==0)) {
						DMap->SetBitsCluster(rootdir[i].first_cluster, DMF_DATA);
						ScanFile(rootdir[i].first_cluster,sbuf,bad_sector_count);
					}
				}
			}
		}
	}
}

u32 FormatDiskIBM::cluster2sector(u32 cls)
{
	bootsect_ibm *boot0 = (bootsect_ibm *)(Disk->GetSector(0));
	u32 ssa = boot0->bpb.reserved_sectors + boot0->bpb.fat_count*boot0->bpb.sectors_per_fat + ((32*boot0->bpb.root_entries)/boot0->bpb.byte_per_sect);
	u32 lsn = ssa+(cls-2)*boot0->bpb.sect_per_cluster;
	return lsn;
}

u32 FormatDiskIBM::clustersize()
{
	bootsect_ibm *boot0 = (bootsect_ibm *)(Disk->GetSector(0));
	return boot0->bpb.sect_per_cluster;
}

char const *FormatDiskIBM::GetDiskTypeString()
{
	switch (DiskType)
	{
		case DT_IBMPC:			return "IBM/PC";
		case DT_ATARIST:		return "Atari ST";
		default:						return "Unknown";
	}
	return "Unknown";
}

char const *FormatDiskIBM::GetDiskSubTypeString()
{
	switch (DiskSubType)
	{
		case DST_3_5DD:			return "3.5\" DD";
		case DST_3_5HD:			return "3.5\" HD";
		case DST_5_25DD:		return "5.25\" DD";
		case DST_5_25HD:		return "5.25\" HD";
		default:						return "Unknown";
	}
	return "Unknown";
}

void FormatDiskIBM::DetectFAT()
{
	bootsect_ibm *boot0 = (bootsect_ibm *)(Disk->GetSector(0));

	// this is according to the official guidelines by microsoft
	u32 root_dir_sects = ((boot0->bpb.root_entries*32)+(boot0->bpb.byte_per_sect-1))/boot0->bpb.byte_per_sect;
	u32 fatsects = 0;
	if (boot0->bpb.sectors_per_fat>0) {
		fatsects = boot0->bpb.sectors_per_fat;
	} else {
		fatsects = boot0->bpbext.bpb32.sectors_per_fat32;
	}
	u32 totalsects = 0;
	if (boot0->bpb.sector_count>0) {
		totalsects = boot0->bpb.sector_count;
	} else {
		totalsects = boot0->bpb.sector_count32;
	}
	u32 datasects = totalsects - (boot0->bpb.reserved_sectors+(boot0->bpb.fat_count*fatsects)+root_dir_sects);

	ClusterCount = datasects / boot0->bpb.sect_per_cluster;

	if (ClusterCount<4085) FatType=FAT12;
	else if (ClusterCount<65525) FatType=FAT16;
	else FatType=FAT32;
}

char const *FormatDiskIBM::FatTypeName(_FatType fattype)
{
	switch (fattype)
	{
		case FAT12: return "FAT12"; break;
		case FAT16: return "FAT16"; break;
		case FAT32: return "FAT32"; break;
	}
	return "Unknown";
}

u32 FormatDiskIBM::GetFATChainNext(u32 cluster)
{
	switch (FatType)
	{
		case FAT12:
			{
				u8 *fatdata = (u8 *)Disk->GetSector(1);
				u32 o = cluster+(cluster>>1);
				u16 fatvalue=0;
				if ((cluster%2)==0)
				{
					// even cluster
					fatvalue = (*((u16 *)(fatdata+o))) & 0xfff;
				} else {
					// odd cluster
					fatvalue = (*((u16 *)(fatdata+o))) >> 4;
				}
				return fatvalue;
			}
			break;
		case FAT16:
			{
				u16 *fatdata = (u16 *)Disk->GetSector(1);
				return fatdata[cluster];
			}
			break;
		case FAT32:
			{
				u32 *fatdata = (u32 *)Disk->GetSector(1);
				return fatdata[cluster]&0x0fffffff;
			}
			break;
	}
	return 0;
}

void FormatDiskIBM::ScanFile(u32 first_cluster, const char *fn, u32 *bad_sector_count)
{
	u32 eoc = 0;
	switch (FatType)
	{
		case FAT12: eoc = 0x0ff8; break;
		case FAT16: eoc = 0xfff8; break;
		case FAT32: eoc = 0x0ffffff8; break;
	}
	u32 cls = first_cluster;
	//printf("%d", cls);
	while ((cls>0)&&(cls<eoc)) {
		DMap->SetBitsCluster(cls, DMF_DATA);
		for (u32 i=0; i<clustersize(); i++)
		{
			u32 s = cluster2sector(cls)+i;
			if ((Disk->IsSectorCRCBad(s))||(Disk->IsSectorCRCBad(s))) 
			{
				clog(1, "# Bad sector %d belonging to file %s found.\n", s, fn);
				if (bad_sector_count) (*bad_sector_count)++;
			}
		}
		cls = GetFATChainNext(cls);
		//printf(", %d", cls);
	}
	//printf("\n");
}