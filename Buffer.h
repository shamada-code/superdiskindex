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

class Buffer
{
public:
	Buffer() : Data(NULL),Size(0),Fill(0) {}
	virtual ~Buffer() { free(Data); }

	void Alloc(u64 size);

	void Push8(u8 db);
	void Push16(u16 dw);
	void Push32(u32 dd);
	void Push64(u64 dl);

	void Clear();

	u8 *GetBuffer() { return Data; }
	u64 GetSize() { return Size; }
	u64 GetFill() { return Fill; }

	void MFMDecode();

protected:
	void RequireSize(u64 size);

	u8 *Data;
	u64 Size;
	u64 Fill;
};
