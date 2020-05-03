///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Global.h"
#include "Types.h"
#include "Helpers.h"
#include "Buffer.h"
#include "TextureTGA.h"

///////////////////////////////////////////////////////////

#pragma pack(1)

struct _tga_header
{
	u8 len_id;
	u8 pal_type;
	u8 img_type;
	u16 pal_start;
	u16 pal_len;
	u8 pal_width;
	u16 originx;
	u16 originy;
	u16 width;
	u16 height;
	u8 bpp;
	u8 attr;
};

#pragma pack(0)

///////////////////////////////////////////////////////////

void mixrgba(u32 *dst, u32 src)
{
	float ad = ((*dst)>>24)/255.0f;
	float bd = (((*dst)>>16)&0xff)/255.0f;
	float gd = (((*dst)>>8)&0xff)/255.0f;
	float rd = (((*dst)>>0)&0xff)/255.0f;
	float as = (src>>24)/255.0f;
	float bs = ((src>>16)&0xff)/255.0f;
	float gs = ((src>>8)&0xff)/255.0f;
	float rs = ((src>>0)&0xff)/255.0f;
	ad = minval(1,as);
	bd = minval(1,bd+as*bs);
	gd = minval(1,gd+as*gs);
	rd = minval(1,rd+as*rs);
	*dst = ((u32)(ad*255.0f)<<24) | ((u32)(bd*255.0f)<<16) | ((u8)(gd*255.0f)<<8) | ((u8)(rd*255.0f)<<0);
}

///////////////////////////////////////////////////////////

TextureTGA::TextureTGA(u16 w, u16 h)
{
	Width=w;
	Height=h;
	BM = new Buffer();
	BM->Alloc(Width*Height*4);
	BM->SetFill(Width*Height*4);
	BM->Zero();
}

TextureTGA::~TextureTGA()
{
	delete(BM);
}

void TextureTGA::Save(char const *fn)
{
	Buffer *BMPal = NULL;

	// count colors & build palette
	BMPal = new Buffer();
	u32 *p0 = (u32 *)BM->GetBuffer();
	u32 pal[256];
	u8 palc=0;

	for (u32 i=0; i<Width*Height; i++)
	{
		bool pal_found = false;
		for (int j=0; j<palc; j++)
		{
			if (p0[i]==pal[j]) pal_found=true;
		}
		if (!pal_found)
		{
			if (palc>=255)
			{
				clog(2, "# TGA: palette overflow!\n");
				//return;
			}
			pal[palc++] = p0[i];
		}
	}
	clog(3, "# TGA: palette has %d entries.\n", palc);

	// open file
	int fd=open(fn, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);

	// create header
	_tga_header header;
	header.len_id=0; // no image id
	header.pal_type=1; // no pal
	header.img_type=9; // indexed,rle compressed
	header.pal_start=0;
	header.pal_len=palc;
	header.pal_width=32;
	header.originx=0;
	header.originy=0;
	header.width=Width;
	header.height=Height;
	header.bpp=8;
	header.attr=0x0 | 0x20; // origin is upper left
	write(fd, &header, sizeof(header));

	// write pal
	for (int j=0; j<palc; j++)
	{
		write(fd, &pal[j], 4);
	}

	// convert to indexed bitmap
	for (u32 i=0; i<Width*Height; i++)
	{
		for (int j=0; j<palc; j++)
		{
			if (p0[i]==pal[j]) BMPal->Push8(j);
		}
	}

	// rle compress
	Buffer rle;
	u8 c=0;
	for (u32 y=0; y<Height; y++)
	{
		u8 *pi0 = BMPal->GetBuffer()+y*Width;
		u8 *pi = pi0;
		while (pi-pi0<Width)
		{
			c=0;
			while ( (c<127) && (pi-pi0<Width) && (*pi==*(pi+1)) )
			{
				c++;
				pi++;
			}
			if (c>0)
			{
				rle.Push8((c)|0x80);
				rle.Push8(*pi++);
			} else {
				c=0;
				rle.Push8(c);
				rle.Push8(*pi++);
			}
		}
		//write(fd, &pi[i*2+1], 1);
	}

	// write rle compressed buffer
	write(fd, rle.GetBuffer(), rle.GetFill());
	
	// write uncompressed indexed buffer
	//write(fd, BMPal->GetBuffer(), BMPal->GetFill());

	// cleanup
	delete(BMPal);

	/*
	u32 *p0 = (u32 *)BM->GetBuffer();
	u32 *p=p0;
	u8 c=0;
	while ((p-p0)<BM->GetFill())
	{
		if (p[0]==p[1])
		{
			//sequence
			while ((c<127)&&((p-p0)<BM->GetFill())&&(*p==*(p+1))) { p++; c++; }
			c|=0x80;
			write(fd, &c, 1);
			write(fd, p, 4);
			c=0;
		} else {
			//singles
			while ((c<127)&&((p-p0)<BM->GetFill())&&(*p!=*(p+1))) { p++; c++; }
			write(fd, &c, 1);
			write(fd, p, 4);
			c=0;
		}
	}
	*/
	
	//write(fd, BM->GetBuffer(), BM->GetFill());
	close(fd);
}

void TextureTGA::Clear(u32 rgba)
{
	u32 *p = (u32 *)BM->GetBuffer();
	for (u32 i=0; i<Width*Height; i++) p[i]=rgba;
}

void TextureTGA::Pixel(u16 x, u16 y, u32 rgba)
{
	u32 *p = (u32 *)BM->GetBuffer();
	mixrgba(&p[y*Width+x],rgba);
}

void TextureTGA::LineH(u16 x0, u16 y0, u16 w, u32 rgba)
{
	u32 *p = (u32 *)BM->GetBuffer();
	for (int i=0; i<w; i++) mixrgba(&p[y0*Width+x0+i],rgba);
}

void TextureTGA::LineV(u16 x0, u16 y0, u16 h, u32 rgba)
{
	u32 *p = (u32 *)BM->GetBuffer();
	for (int i=0; i<h; i++) mixrgba(&p[(y0+i)*Width+x0],rgba);
}

void TextureTGA::Rect(u16 x0, u16 y0, u16 w, u16 h, u32 rgba)
{
	u32 *p = (u32 *)BM->GetBuffer();
	for (int y=0; y<h; y++) for (int x=0; x<w; x++) mixrgba(&p[(y0+y)*Width+x0+x],rgba);
}
