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
	return 0x1900*2;

}

///////////////////////////////////////////////////////////

// bool FormatDiskAmiga::Detect()
// {

// 	return false;
// }

void FormatDiskAmiga::HandleBlock(Buffer *buffer)
{
	//printf("Handling Block with size %d.\n", buffer->GetFill());
	buffer->MFMDecode();
	// u8 *db = buffer->GetBuffer();
	// if ( (db[0]==0xa1) && (db[1]==0xfe) )
	// {
	// 	if (Config.verbose>=2) printf("# Sector Header + %02d/%01d/%02d + %04x\n", db[2], db[3], db[4], (db[6]<<8)|db[7]);
	// }
	// if ( (db[0]==0xa1) && (db[1]==0xfb) )
	// {
	// 	if (Config.verbose>=2) printf("# Sector Data + %04x\n", (db[2+512+0]<<8)|db[2+512+1]);
	// }
	//u16 *p16 = (u16 *)(buffer->GetBuffer());
	int sc = buffer->GetFill()/0x220;
	for (int s=0; s<sc; s++)
	{
		u16 *p16 = (u16 *)(buffer->GetBuffer())+(s*0x110);
		u32 info = Weave32(swap16(p16[1]), swap16(p16[2]));
		u32 label0 = Weave32(swap16(p16[3]), swap16(p16[7]));
		u32 label1 = Weave32(swap16(p16[4]), swap16(p16[8]));
		u32 label2 = Weave32(swap16(p16[5]), swap16(p16[9]));
		u32 label3 = Weave32(swap16(p16[6]), swap16(p16[10]));
		u32 crc_head = Weave32(swap16(p16[11]), swap16(p16[12]));
		u32 crc_data = Weave32(swap16(p16[13]), swap16(p16[14]));
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
		if (Config.verbose>=2) printf("# Sector info + %02x + %02x + %02x + %02x\n", disktype, disktrack, disksect, diskleft );
		if (Config.verbose>=2) printf("# Sector label + %08x\n", label0 );
		if (Config.verbose>=2) printf("# Sector label + %08x\n", label1 );
		if (Config.verbose>=2) printf("# Sector label + %08x\n", label2 );
		if (Config.verbose>=2) printf("# Sector label + %08x\n", label3 );

		if (disktype!=0xff) continue;

		LastCyl = max(LastCyl, disktrack>>1);
		LastSect = max(LastSect, disksect);

		if (Config.verbose>=2) hexdump(sect_data.GetBuffer(), sect_data.GetFill());

		if (Disk!=NULL)
		{
			Disk->AddSector(disktrack>>1, disktrack&1, disksect, sect_data.GetBuffer(), sect_data.GetFill());
		}
	}
}

bool FormatDiskAmiga::Analyze()
{
	SectorBoot0 *boot0 = (SectorBoot0 *)Disk->GetSector(0);
	if (memcmp(boot0->magic_dos, "DOS", 3)==0) printf("# ANALYZE: boot block magic ok.\n");
	else return false;

	SectorRoot *root = (SectorRoot *)Disk->GetSector(swap(boot0->rootblock));
	printf("0x%x\n", swap(root->type));
	if (swap(root->type)==0x2) printf("# ANALYZE: root block type ok.\n");
	else return false;

	char sbuf[32]; strncpy(sbuf, root->name, root->name_len);
	printf("# ANALYZE: volume label is '%s'.\n", sbuf);

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
