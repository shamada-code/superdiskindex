///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "Helpers.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

u16 swap16(u16 v) { return ( ((((v)>>8)&0xff)<<0) | ((((v)>>0)&0xff)<<8) ); }

u8 printablechar(u8 b) { return (b<0x20)?'.':((b>=0x80)?'.':b); }

void hexdump(void *dataptr, u32 len)
{
	u8 *data=(u8 *)dataptr;
	int o=0;
	if ((len%16)>0) o=1;
	for (u32 i=0; i<(len>>4)+o; i++)
	{
		clog(3,"%08x  |  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  | %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c\n",
			i*0x10,
			(i*0x10+0x0<len)?data[i*0x10+0x0]:0,
			(i*0x10+0x1<len)?data[i*0x10+0x1]:0,
			(i*0x10+0x2<len)?data[i*0x10+0x2]:0,
			(i*0x10+0x3<len)?data[i*0x10+0x3]:0,
			(i*0x10+0x4<len)?data[i*0x10+0x4]:0,
			(i*0x10+0x5<len)?data[i*0x10+0x5]:0,
			(i*0x10+0x6<len)?data[i*0x10+0x6]:0,
			(i*0x10+0x7<len)?data[i*0x10+0x7]:0,
			(i*0x10+0x8<len)?data[i*0x10+0x8]:0,
			(i*0x10+0x9<len)?data[i*0x10+0x9]:0,
			(i*0x10+0xa<len)?data[i*0x10+0xa]:0,
			(i*0x10+0xb<len)?data[i*0x10+0xb]:0,
			(i*0x10+0xc<len)?data[i*0x10+0xc]:0,
			(i*0x10+0xd<len)?data[i*0x10+0xd]:0,
			(i*0x10+0xe<len)?data[i*0x10+0xe]:0,
			(i*0x10+0xf<len)?data[i*0x10+0xf]:0,
			(i*0x10+0x0<len)?printablechar(data[i*0x10+0x0]):' ',
			(i*0x10+0x1<len)?printablechar(data[i*0x10+0x1]):' ',
			(i*0x10+0x2<len)?printablechar(data[i*0x10+0x2]):' ',
			(i*0x10+0x3<len)?printablechar(data[i*0x10+0x3]):' ',
			(i*0x10+0x4<len)?printablechar(data[i*0x10+0x4]):' ',
			(i*0x10+0x5<len)?printablechar(data[i*0x10+0x5]):' ',
			(i*0x10+0x6<len)?printablechar(data[i*0x10+0x6]):' ',
			(i*0x10+0x7<len)?printablechar(data[i*0x10+0x7]):' ',
			(i*0x10+0x8<len)?printablechar(data[i*0x10+0x8]):' ',
			(i*0x10+0x9<len)?printablechar(data[i*0x10+0x9]):' ',
			(i*0x10+0xa<len)?printablechar(data[i*0x10+0xa]):' ',
			(i*0x10+0xb<len)?printablechar(data[i*0x10+0xb]):' ',
			(i*0x10+0xc<len)?printablechar(data[i*0x10+0xc]):' ',
			(i*0x10+0xd<len)?printablechar(data[i*0x10+0xd]):' ',
			(i*0x10+0xe<len)?printablechar(data[i*0x10+0xe]):' ',
			(i*0x10+0xf<len)?printablechar(data[i*0x10+0xf]):' '
			);
	}
}

/////////////////

int __clog_fd=-1;

void clog(int lvl, char const *fmt, ...)
{
	va_list args;
	if ((Config.gen_log)&&(__clog_fd>=0)&&(2>=lvl))
	{
		va_start(args,fmt);
		vdprintf(__clog_fd, fmt, args);
		va_end(args);
	}
	if (Config.verbose>=lvl) {
		va_start(args,fmt);
		vprintf(fmt, args);
		va_end(args);
	}
}

void clog_init()
{
	if ((Config.gen_log)&&(strlen(Config.fn_out)>0))
	{
		char sbuf[65100];
		sprintf(sbuf, "%s.log", Config.fn_out);
	__clog_fd = open(sbuf, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);
	}
}

void clog_exit()
{
	if (clog>=0) close(__clog_fd);
}
