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
	u8 *db = (u8 *)(Disk->GetSector(0));
	hexdump(db, 512);
	return false;
}
