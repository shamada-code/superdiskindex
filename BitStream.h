///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

///////////////////////////////////////////////////////////

struct streamblockdef
{
	u32 SyncWord;
	u32 PayloadLength;
};

///////////////////////////////////////////////////////////

class BitStream
{
public:
	BitStream(class Format *fmt);
	virtual ~BitStream();

	void InitSyncWords();
	void AddSyncWord(u32 dw, u32 payload_length);

	void Feed(u8 bit);
	void LostSync();
	void Flush();

	class Buffer *GetBuffer() { return Data; }

protected:
	void Store(u8 val);

	class Format *Fmt;
	class Buffer *Data;
	u32 Capture;
	u8 CaptureUsed;
	u8 const CaptureWidth = sizeof(Capture);
	int SyncFlag;
	streamblockdef SyncDefs[8];
	int SyncDefCount;
	int ActiveSyncDef;
	u32 PayloadCounter;
};
