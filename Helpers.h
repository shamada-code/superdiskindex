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

#define maxval(a,b) ((a)>(b)?(a):(b))
#define minval(a,b) ((a)<(b)?(a):(b))

u16 swap16(u16 v);
u8 printablechar(u8 b);
void hexdump(void *data, u32 len);

inline u8 swap(u8 v) { return v; }
inline u16 swap(u16 v) { return ( ((((v)>>8)&0xff)<<0) | ((((v)>>0)&0xff)<<8) ); }
inline u32 swap(u32 v) { return ( ((((v)>>24)&0xff)<<0) | ((((v)>>16)&0xff)<<8) | ((((v)>>8)&0xff)<<16) | ((((v)>>0)&0xff)<<24) ); }

#define countof(x) (sizeof(x)/sizeof(*x))

void clog(int lvl, char const *fmt, ...);
void clog_init();
void clog_exit();

enum OutputTypes
{
	OT_LOG,
	OT_LISTING,
	OT_DISKIMAGE,
	OT_MAPS,
	OT_FLUXVIZ
};

struct OutputParams
{
	OutputParams(char const *_platform, u16 _track, u16 _head, u16 _rev, u16 _quality) { platform=_platform; track=_track; head=_head; rev=_rev; quality=_quality; }
	char const *platform;
	u16 track;
	u16 head;
	u16 rev;
	u16 quality;
};

// fmt vars:
// @i : input basename
// @t : output type (listing,maps,diskimage,...)
// @p : platform
// @q : quality level
void gen_output_filename(char **out, char const *fmt, OutputTypes type, char const *ext, OutputParams const &params);

void rec_mkdir(char const *fn);
