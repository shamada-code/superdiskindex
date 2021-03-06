///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include "Types.h"

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
	BitStream(class Format *fmt, u8 currev);
	virtual ~BitStream();

	void InitSyncWords();
	void AddSyncWord(u32 dw, u32 payload_length);

	void SetRev(u8 currev) { CurRev=currev; }

	void Feed(u8 bit);
	void LostSync();
	void Flush();

	class Buffer *GetBuffer() { return Data; }

	bool IsSynced() { return SyncFlag==1; }
	u8 GetActiveSyncDef() { return ActiveSyncDef; }

	void EnableRawBitstream();
	void DisableRawBitstream();
	void ResetRawBitstream();
	class Buffer *GetRawBuffer();

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
	u8 CurRev;

	class Buffer *RawTrack;
	u8 RawTrackSR;
	u8 RawTrackC;
};
