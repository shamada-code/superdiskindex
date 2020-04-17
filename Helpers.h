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

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

u16 swap16(u16 v);
u8 printablechar(u8 b);
void hexdump(void *data, u32 len);

inline u16 swap(u16 v) { return ( ((((v)>>8)&0xff)<<0) | ((((v)>>0)&0xff)<<8) ); }
inline u32 swap(u32 v) { return ( ((((v)>>24)&0xff)<<0) | ((((v)>>16)&0xff)<<8) | ((((v)>>8)&0xff)<<16) | ((((v)>>0)&0xff)<<24) ); }

#define countof(x) (sizeof(x)/sizeof(*x))

void clog(int lvl, char const *fmt, ...);
void clog_init();
void clog_exit();
