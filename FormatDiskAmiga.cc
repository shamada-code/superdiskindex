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
#include "FormatDiskAmiga.h"
#include "Buffer.h"
#include "MFM.h"
#include "Helpers.h"
#include "VirtualDisk.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "CRC.h"
#include "DiskMap.h"

///////////////////////////////////////////////////////////

enum DiskTypes
{
	DT_UNKNOWN = 0,
	DT_AMIGA,
};

enum DiskSubTypes
{
	DST_UNKNOWN = 0,
	DST_3_5DD,
	DST_3_5HD,
};

///////////////////////////////////////////////////////////

#pragma pack(1)

struct SectorBoot0
{
	u8 magic_dos[3];
	u8 flags;
	u32 crc;
	u32 rootblock;
	u8 code[512-12];
};

struct SectorRoot
{
	u32 type;
	u32 header_key;
	u32 high_seq;
	u32 ht_size;
	u32 first_data;
	u32 crc;
	u32 ht[0x48];
	u32 bm_flag;
	u32 bm_pages[25];
	u32 bm_ext;
	u32 r_days;
	u32 r_mins;
	u32 r_ticks;
	u8 name_len;
	char name[30];
	u8 unused_0;
	u32 unused_1;
	u32 v_days;
	u32 v_mins;
	u32 v_ticks;
	u32 c_days;
	u32 c_mins;
	u32 c_ticks;
	u32 next_hash;
	u32 parent_dir;
	u32 extension;
	u32 sec_type;
};

struct SectorFileHead
{
	u32 type;
	u32 header_key;
	u32 high_seq;
	u32 data_size;
	u32 first_data;
	u32 crc;
	u32 data_blocks[0x48];
	u32 unused_0;
	u16 uid;
	u16 gid;
	u32 protect;
	u32 size;
	u8 comment_len;
	char comment[79];
	u8 unused_1[12];
	u32 days;
	u32 mins;
	u32 ticks;
	u8 name_len;
	char name[30];
	u8 unused_2;
	u32 unused_3;
	u32 real_entry;
	u32 next_link;
	u32 unused_4[5];
	u32 next_hash;
	u32 parent_dir;
	u32 extension;
	u32 sec_type;
};

#pragma pack(0)

///////////////////////////////////////////////////////////

char const *FormatDiskAmiga::GetName() { return "Amiga"; }

u8 FormatDiskAmiga::GetSyncWordCount()
{
	return 1;
}

u32 FormatDiskAmiga::GetSyncWord(int n)
{
	return 0x44894489;
}

u32 FormatDiskAmiga::GetSyncBlockLen(int n)
{
	return 0x438;//0x1900*2;

}

///////////////////////////////////////////////////////////

// bool FormatDiskAmiga::Detect()
// {

// 	return false;
// }

void FormatDiskAmiga::HandleBlock(Buffer *buffer, int currev)
{
	//printf("Handling Block with size %d ($%08x).\n", buffer->GetFill(), buffer->GetFill());
	// u32 expected_size = (0x1900*2+4);
	// if (buffer->GetFill()!=expected_size)
	// {
	//  	clog(2,"# Wrong track size ($%04x!=$%04x)\n", buffer->GetFill(), expected_size);
	// 	return;
	// }
	Buffer *rawbuffer = new Buffer(*buffer);
	//printf("Handling Block with size %d.\n", buffer->GetFill());
	buffer->MFMDecode();
	// u8 *db = buffer->GetBuffer();
	// if ( (db[0]==0xa1) && (db[1]==0xfe) )
	// {
	// 	clog(2,"# Sector Header + %02d/%01d/%02d + %04x\n", db[2], db[3], db[4], (db[6]<<8)|db[7]);
	// }
	// if ( (db[0]==0xa1) && (db[1]==0xfb) )
	// {
	// 	clog(2,"# Sector Data + %04x\n", (db[2+512+0]<<8)|db[2+512+1]);
	// }
	//u16 *p16 = (u16 *)(buffer->GetBuffer());
	int sc = max(1,buffer->GetFill()/0x220);
	for (int s=0; s<sc; s++)
	{
		u16 *p16 = (u16 *)(buffer->GetBuffer())+(s*0x110);
		u32 info = Weave32(swap16(p16[1]), swap16(p16[2]));
		//u32 label0 = Weave32(swap16(p16[3]), swap16(p16[7]));
		//u32 label1 = Weave32(swap16(p16[4]), swap16(p16[8]));
		//u32 label2 = Weave32(swap16(p16[5]), swap16(p16[9]));
		//u32 label3 = Weave32(swap16(p16[6]), swap16(p16[10]));
		u32 crc_head = Weave32(swap16(p16[11]), swap16(p16[12]));
		CRC32_xor crc_head_calc(0);
		crc_head_calc.Block(rawbuffer->GetBuffer()+(s*0x440+4), 10, true);

		u32 crc_data = Weave32(swap16(p16[13]), swap16(p16[14]));
		CRC32_xor crc_data_calc(0);
		crc_data_calc.Block(rawbuffer->GetBuffer()+(s*0x440+60), 256, true);
		//u32 crc_head_calc = chksum32(rawbuffer->GetBuffer()+(s*0x440+4), 10)&0x55555555;
		//u32 crc_data_calc = chksum32(rawbuffer->GetBuffer()+(s*0x440+60), 256)&0x55555555;

		//printf("#$CRCh: %08x / %08x / %08x\n", crc_head, crc_head_calc, crc_head^crc_head_calc);
		//printf("#$CRCd: %08x / %08x / %08x\n", crc_data, crc_data_calc, crc_data^crc_data_calc);
		bool crc1ok = (crc_head^(crc_head_calc.Get()&0x55555555))==0;
		bool crc2ok = (crc_data^(crc_data_calc.Get()&0x55555555))==0;
		Buffer sect_data;
		for (int i=0; i<128; i++)
		{
			u32 v = Weave32(swap16(p16[15+i]), swap16(p16[15+128+i]));
			sect_data.Push32(v);
		}
		u8 disktype = ((info>>24)&0xff);
		u8 disktrack = ((info>>16)&0xff);
		u8 disksect = ((info>>8)&0xff);
		u8 diskleft = ((info>>0)&0xff);
		clog(2,"# Sector info + %02x + %02x + %02x + %02x\n", disktype, disktrack, disksect, diskleft );
		clog(2,"# Sector header chksum %s\n", crc1ok?"OK":"BAD" );
		clog(2,"# Sector data chksum %s\n", crc2ok?"OK":"BAD" );
		//clog(2,"# Sector label + %08x\n", label0 );
		//clog(2,"# Sector label + %08x\n", label1 );
		//clog(2,"# Sector label + %08x\n", label2 );
		//clog(2,"# Sector label + %08x\n", label3 );

		if (disktype!=0xff) continue;
		if (disktrack>=164) continue; // either a very weird format or bad data

		if (crc1ok)
		{
			LastCyl = max(LastCyl, disktrack>>1);
			LastSect = max(LastSect, disksect);
		}

		if (Config.verbose>=3) hexdump(sect_data.GetBuffer(), sect_data.GetFill());

		if ((Disk!=NULL)&&(crc1ok))
		{
			Disk->AddSector(disktrack>>1, disktrack&1, disksect, currev, sect_data.GetBuffer(), sect_data.GetFill(), crc1ok, crc2ok);
		}
	}
	delete(rawbuffer);
}

bool FormatDiskAmiga::Analyze()
{
	SectorBoot0 *boot0 = (SectorBoot0 *)Disk->GetSector(0);
	if (memcmp(boot0->magic_dos, "DOS", 3)==0) clog(1,"# ANALYZE: boot block magic ok.\n");
	else {
		clog(1,"# ANALYZE: boot block magic FAILED ('%c%c%c'!='%c%c%c').\n",boot0->magic_dos[0], boot0->magic_dos[1], boot0->magic_dos[2], 'D','O','S');
		return false;
	}

	u32 rootblkidx = swap(boot0->rootblock);
	if ((rootblkidx==0)||(rootblkidx>Disk->GetSectorCount()))
	{
		clog(2, "# Warning: Rootblock setting in Bootblock is bad. Using default location for root block.\n");
		if (Disk->GetLayoutSectors()==11) rootblkidx=880; // DD Disk
		else if (Disk->GetLayoutSectors()==22) rootblkidx=1760; // HD Disk
		else rootblkidx=880; // assume DD Disk?
	}
	SectorRoot *root = (SectorRoot *)Disk->GetSector(rootblkidx);
	if (swap(root->type)==0x2) clog(1,"# ANALYZE: root block type ok.\n");
	else {
		clog(1,"# ANALYZE: root block @ %d type FAILED ($%08x!=$%08x).\n", swap(boot0->rootblock), swap(root->type),0x2);
		hexdump(root, 512);
		return false;
	}

	char sbuf[32]; strncpy(sbuf, root->name, root->name_len);
	clog(1,"# ANALYZE: volume label is '%s'.\n", sbuf);

	// if we got this far, this probably is a proper Amiga disk
	SetDiskType(DT_AMIGA);
	if (Disk->GetLayoutSectors()==11) SetDiskSubType(DST_3_5DD);
	if (Disk->GetLayoutSectors()==22) SetDiskSubType(DST_3_5HD);

	// Initialize DiskMap
	{
		DMap = new DiskMap(Disk->GetSectorCount());
		DMap->SetBitsSector(0, DMF_BOOTBLOCK);
		DMap->SetBitsSector(rootblkidx, DMF_ROOTBLOCK);
		clog(2, "# Bitmap settings: Ext=%d, Flag=%d, Pages=%d,%d,%d,...\n", swap(root->bm_ext), swap(root->bm_flag), swap(root->bm_pages[0]), swap(root->bm_pages[1]), swap(root->bm_pages[2]));
		for (u32 i=0; i<countof(root->bm_pages); i++)
		{
			if (swap(root->bm_pages[i])>0)
			{
				DMap->SetBitsSector(swap(root->bm_pages[i]), DMF_BLOCKMAP);
			}
		}
		for (int i=0; i<Disk->GetSectorCount(); i++)
		{
			if (Disk->IsSectorMissing(i)) DMap->SetBitsSector(i, DMF_MISSING);
			if (Disk->IsSectorCRCBad(i)) DMap->SetBitsSector(i, DMF_CRC_LOWLEVEL_BAD);
		}
	}

	// Listing
	{
		int fd=-1;
		clog(2,"# Loading rootblock @ %d.\n",rootblkidx);

		if (Config.gen_listing)
		{
			char fnbuf[65100]; snprintf(fnbuf, sizeof(fnbuf), "%s.lst", Config.fn_out);
			clog(1,"# Generating file listing '%s'.\n",fnbuf);
			fd = open(fnbuf, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);
		}
		if ((Config.gen_listing)&&(fd>=0))
		{
			dprintf(fd, "Volume: %s\n", sbuf);
			dprintf(fd, "\n");
			dprintf(fd, "%-60s Type    Size  Blk  UID  GID\n", "Filename");
			dprintf(fd, "-----------------------------------------------------------------------------------------\n");
		}
		ParseDirectory(fd, rootblkidx,"/");
		if ((Config.gen_listing)&&(fd>=0))
		{
			close(fd);
		}
	}

	// compare DiskMap with on-disk blockmap
	{
		u32 blkmapidx = swap(root->bm_pages[0]);
		u32 *blkmap = (u32 *)Disk->GetSector(blkmapidx);
		//clog(1, "BLOCKMAP:\n# %04d |   ", 0);
		bool blkmapmatch=true;
		for (int i=0; i<Disk->GetSectorCount()/32; i++)
		{
			u32 bmval = swap(blkmap[i+1]);
			for (int j=0; j<32; j++)
			{
				//clog(1, "%c", bmval&(1<<j)?'.':'+');
				//if ((i*32+j+2)%128==127) clog(1, "\n# %04d | ", (i+1)*32);
				u32 sectidx = i*32+j+2;
				if (sectidx<Disk->GetSectorCount())
				{
					u8 sect_free = ((bmval&(1<<j))>0);
					if ( (sect_free) && (DMap->GetSector(sectidx, DMF_CONTENT_MASK)>0) )
					{
						clog(1,"# WARNING: BlockMap mismatch - Sector %d is marked free, but belongs to a file/directory!\n", sectidx);
						blkmapmatch=false;
					}
					if ( (!sect_free) && (DMap->GetSector(sectidx, DMF_CONTENT_MASK)==0) )
					{
						clog(1,"# WARNING: BlockMap mismatch - Sector %d is marked used, but doesn't belong to a file/directory!\n", sectidx);
						blkmapmatch=false;
					}
				}
			}
		}
		if (blkmapmatch) clog(1, "# Blockmap matches scanned files/directories on disk.\n");
	}

	// Output DiskMaps
	{
		if (Config.gen_maps)
		{
			char fnbuf[65100]; snprintf(fnbuf, sizeof(fnbuf), "%s.maps", Config.fn_out);
			clog(1,"# Generating block/usage/healthmaps '%s'.\n",fnbuf);
			if (DMap)
			{
				DMap->OutputMaps(fnbuf);
			}
		}
	}

	// Export
	if (Config.gen_export)
	{
		char fnbuf[65100]; snprintf(fnbuf, sizeof(fnbuf), "%s.adf", Config.fn_out);
		clog(1,"# Generating diskimage '%s'.\n",fnbuf);
		if (Disk)
		{
			Disk->ExportADF(fnbuf);
		}
	}
	return true;
}

u16 FormatDiskAmiga::Spread8(u8 data)
{
	return (
		(((u16)data&0x80)<<7) |
		(((u16)data&0x40)<<6) |
		(((u16)data&0x20)<<5) |
		(((u16)data&0x10)<<4) |
		(((u16)data&0x8)<<3) |
		(((u16)data&0x4)<<2) |
		(((u16)data&0x2)<<1) |
		(((u16)data&0x1)<<0)
		);
}
u32 FormatDiskAmiga::Spread16(u16 data) { return (((u32)Spread8(data>>8)<<16)|((u32)Spread8(data&0xff)<<0)); }
u64 FormatDiskAmiga::Spread32(u32 data) { return (((u64)Spread16(data>>16)<<32)|((u64)Spread16(data&0xffff)<<0)); }

u32 FormatDiskAmiga::Weave32(u16 odd, u16 even)
{
	return (Spread16(odd)<<1)|Spread16(even);
}

void FormatDiskAmiga::ParseDirectory(int fd, u32 block, char const *prefix)
{
	SectorFileHead *base = (SectorFileHead *)Disk->GetSector(block);

	for (u32 i=0; i<countof(base->data_blocks); i++)
	{
		u32 blk = swap(base->data_blocks[i]);
		while ((blk>0)&&(blk<Disk->GetSectorCount()))
		{
			clog(2,"# Loading fileheader @ %d.\n",blk);
			SectorFileHead *filehead = (SectorFileHead *)Disk->GetSector(blk);
			//hexdump(filehead, 512);
			char sbuf[30]; 
			memset(sbuf, 0, sizeof(sbuf)); 
			strncpy(sbuf, filehead->name, filehead->name_len);
			char sbuf2[8192];
			memset(sbuf2, 0, sizeof(sbuf2));
			sprintf(sbuf2, "%s%s%s", prefix, sbuf, swap(filehead->sec_type)==2?"/":"");
			char sectype='?';
			switch (swap(filehead->sec_type))
			{
				case -3: sectype='F'; DMap->SetBitsSector(blk, DMF_FILEHEADER); break;
				case 2: sectype='D'; DMap->SetBitsSector(blk, DMF_DIRECTORY); break;
			}
			//clog(1, "dmap setting blk %04d to %c\n", blk, sectype);
			if ((Config.gen_listing)&&(fd>=0))
			{
				dprintf(fd, "%-60s [%c] %8d %04d %4d %4d\n", sbuf2,sectype,swap(filehead->size), blk, swap(filehead->uid), swap(filehead->gid));
			}
			if (swap(filehead->sec_type)==2)
			{
				ParseDirectory(fd, blk, sbuf2);
			}
			if (swap(filehead->sec_type)==-3)
			{
				ScanFile(blk);
			}

			blk=swap(filehead->next_hash);
			if (blk>0) clog(2, "# following hash_chain to blk @ %d\n", blk);
		}
	}
}

// u32 FormatDiskAmiga::chksum32(u8 *p, u32 count)
// {
// 	u32 crc = 0;
// 	for (u32 i=0; i<count; i++)
// 	{
// 		crc ^= (((u32)(p[i*4+0]))<<24) | (((u32)(p[i*4+1]))<<16) | (((u32)(p[i*4+2]))<<8) | (((u32)(p[i*4+3]))<<0);
// 	}
// 	return crc;
// }

char const *FormatDiskAmiga::GetDiskTypeString()
{
	switch (DiskType)
	{
		case DT_AMIGA:		return "Amiga";
		default:					return "Unknown";
	}
	return "Unknown";
}

char const *FormatDiskAmiga::GetDiskSubTypeString()
{
	switch (DiskSubType)
	{
		case DST_3_5DD:			return "3.5\" DD";
		case DST_3_5HD:			return "3.5\" HD";
		default:						return "Unknown";
	}
	return "Unknown";
}

void FormatDiskAmiga::ScanFile(u32 block)
{
	SectorFileHead *base = (SectorFileHead *)Disk->GetSector(block);
	char sbuf[30]; 
	memset(sbuf, 0, sizeof(sbuf)); 
	strncpy(sbuf, base->name, base->name_len);
	clog(2, "scanning file %04d with name '%s'\n", block, sbuf);

	clog(2, "blocks: ");
	for (u32 i=0; i<countof(base->data_blocks); i++)
	{
		u32 datablk = swap(base->data_blocks[i]);
		if (datablk>0)
		{
			clog(2, "%d, ", datablk);
			DMap->SetBitsSector(datablk, DMF_DATA);
			//u32 blk = swap(base->data_blocks[i]);
			//while ((blk>0)&&(blk<Disk->GetSectorCount()))
			//{
			//}
			if (DMap->GetSector(datablk, DMF_HEALTH_MASK)>0)
			{
				clog(1, "# Bad sector %04d found (belongs to file '%s')\n", block, sbuf);
			}
		}
	}
	u32 ext = swap(base->extension);
	if (ext>0)
	{
		DMap->SetBitsSector(ext, DMF_FILEHEADER);
		ScanFile(ext);
	}
	clog(2, "\n");
}
