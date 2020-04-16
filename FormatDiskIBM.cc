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

void FormatDiskIBM::HandleBlock(Buffer *buffer)
{
	//printf("Handling Block with size %d.\n", buffer->GetFill());
	buffer->MFMDecode();
	u8 *db = buffer->GetBuffer();
	if ( (db[0]==0xa1) && (db[1]==0xfe) )
	{
		if (Config.verbose>=2) printf("# Sector Header + %02d/%01d/%02d + %04x\n", db[2], db[3], db[4], (db[6]<<8)|db[7]);
	}
	if ( (db[0]==0xa1) && (db[1]==0xfb) )
	{
		if (Config.verbose>=2) printf("# Sector Data + %04x\n", (db[2+512+0]<<8)|db[2+512+1]);
	}
	//hexdump(buffer->GetBuffer(), min(buffer->GetFill(), 64));
}

bool FormatDiskIBM::Analyze()
{
	return false;
}
