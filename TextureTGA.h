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

class TextureTGA
{
public:
	TextureTGA(u16 w, u16 h);
	virtual ~TextureTGA();

	void Save(char const *fn);

	void Clear(u32 rgba);
	void Pixel(u16 x, u16 y, u32 rgba);
	void LineH(u16 x0, u16 y0, u16 w, u32 rgba);
	void LineV(u16 x0, u16 y0, u16 h, u32 rgba);
	void Rect(u16 x0, u16 y0, u16 x1, u16 y1, u32 rgba);

protected:
	u16 Width;
	u16 Height;
	class Buffer *BM;
};