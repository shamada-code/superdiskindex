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

	Buffer(Buffer const &src) 
	{
		Data=NULL;
		Size=0;
		Fill=0;

		Alloc(src.Size);
		memcpy(Data, src.Data, src.Fill);
		Fill=src.Fill;
	}

	void Alloc(u64 size);
	void SetFill(u64 fill); // set fill manually (if memcpy'ing data into buffer)

	void Push8(u8 db);
	void Push16(u16 dw);
	void Push32(u32 dd);
	void Push64(u64 dl);

	void Clear();

	void Zero();

	u8 *GetBuffer() { return Data; }
	u64 GetSize() { return Size; }
	u64 GetFill() { return Fill; }

	void MFMDecode();
	void GCRDecode(u32 byteoffset=0, u8 bitoffset=0);

protected:
	void RequireSize(u64 size);

	u8 *Data;
	u64 Size;
	u64 Fill;
};
