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
#include "Buffer.h"
#include "Format.h"
#include "Helpers.h"

BitStream::BitStream(Format *fmt, u8 currev)
{
	Data=new Buffer();
	Capture=0;
	CaptureUsed=0;
	SyncDefCount=0;
	SyncFlag=0;
	ActiveSyncDef=-1;
	PayloadCounter=0;
	Fmt=fmt;
	CurRev=currev;
}

BitStream::~BitStream()
{
	delete(Data);
}

void BitStream::InitSyncWords()
{
	if (Fmt)
	{
		for (int i=0; i<Fmt->GetSyncWordCount(); i++)
		{
			AddSyncWord(Fmt->GetSyncWord(i), Fmt->GetSyncBlockLen(i));
		}
	}
}

void BitStream::AddSyncWord(u32 dw, u32 payload_len)
{
	SyncDefs[SyncDefCount].SyncWord = dw;
	SyncDefs[SyncDefCount].PayloadLength = payload_len;
	SyncDefCount++;
}

void BitStream::LostSync()
{
	SyncFlag=0;
	ActiveSyncDef=-1;
	PayloadCounter=0;
}

void BitStream::Feed(u8 bit)
{
	Capture = (Capture<<1) | bit;
	CaptureUsed++;

	if (SyncFlag==0)
	{
		// looking for sync
		for (int i=0; i<SyncDefCount; i++)
		{
			if (Capture==SyncDefs[i].SyncWord)
			{
				SyncFlag=1;
				CaptureUsed=0;
				Capture=0;
				ActiveSyncDef=i;
				PayloadCounter=0;
				Store( (SyncDefs[i].SyncWord>>24)&0xff );
				Store( (SyncDefs[i].SyncWord>>16)&0xff );
				Store( (SyncDefs[i].SyncWord>>8)&0xff );
				Store( (SyncDefs[i].SyncWord>>0)&0xff );
				clog(3,"#SYNC\n");
			}
		}
	} else {
		// we are synced
		if (CaptureUsed>=8)
		{
			u8 db = Capture>>(CaptureUsed-8);
			Capture = Capture&((~0)>>(CaptureWidth-(CaptureUsed-8)));
			CaptureUsed-=8;
			Store(db);
			PayloadCounter++;
			if (PayloadCounter>=SyncDefs[ActiveSyncDef].PayloadLength)
			{
				Fmt->HandleBlock(Data, CurRev);
				Data->Clear();
				LostSync();
			}
		}
	}
}

void BitStream::Store(u8 val)
{
	//clog(3,"#RAW: %02x\n", val);
	Data->Push8(val);
}

void BitStream::Flush()
{
	if (SyncFlag>0)
	{
		Fmt->HandleBlock(Data, CurRev);
	}
}
