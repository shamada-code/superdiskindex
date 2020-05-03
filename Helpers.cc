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

#include <string>

u16 swap16(u16 v) { return ( ((((v)>>8)&0xff)<<0) | ((((v)>>0)&0xff)<<8) ); }

u8 printablechar(u8 b) { return (b<0x20)?'.':((b>=0x80)?'.':b); }

void hexdump(void *dataptr, u32 len)
{
	u8 *data=(u8 *)dataptr;
	int o=0;
	if ((len%16)>0) o=1;
	for (u32 i=0; i<(len>>4)+o; i++)
	{
		#if DEBUG_HEXDUMP
		clog(0,"%08x  |  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  | %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c\n",
		#else
		clog(3,"%08x  |  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  | %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c\n",
		#endif
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
		char *fnout=NULL;
		gen_output_filename(&fnout, Config.fn_out, OT_LOG, ".log", OutputParams("unknown", 0,0,0,0));
	__clog_fd = open(fnout, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);
		free(fnout);
	}
}

void clog_exit()
{
	if (__clog_fd>=0) close(__clog_fd);
}

void gen_output_filename(char **out, char const *fmt, OutputTypes type, char const *ext, OutputParams const &params)
{
	char numbuf[64];
	std::string tmp(fmt);
	// @i : input basename
	std::string basefin(Config.fn_in);
	basefin = basefin.substr(basefin.find_last_of('/')+1);
	basefin = basefin.substr(0, basefin.length()-4);
	while (tmp.find("@i")!=std::string::npos) { tmp.replace(tmp.find("@i"), 2, basefin); }
	// @t : output type (listing,maps,diskimage,...)
	char const *otname="_";
	switch (type)
	{
		case OT_DISKIMAGE: otname="disk"; break;
		case OT_FLUXVIZ: otname="fluxviz"; break;
		case OT_LISTING: otname="listing"; break;
		case OT_MAPS: otname="maps"; break;
		case OT_LOG: otname="log"; break;
	}
	while (tmp.find("@t")!=std::string::npos) { tmp.replace(tmp.find("@t"), 2, otname); }
	// @p : platform
	while (tmp.find("@p")!=std::string::npos) { tmp.replace(tmp.find("@p"), 2, params.platform); }
	// @q : quality level
	sprintf(numbuf, "%03d", params.quality);
	while (tmp.find("@q")!=std::string::npos) { tmp.replace(tmp.find("@q"), 2, numbuf); }

	// append t/h/r
	if (type==OT_FLUXVIZ)
	{
		sprintf(numbuf, ".T%03d", params.track);
		tmp+=numbuf;
		sprintf(numbuf, ".H%01d", params.head);
		tmp+=numbuf;
		sprintf(numbuf, ".R%02d", params.rev);
		tmp+=numbuf;
	}

	tmp+=ext;

	rec_mkdir(tmp.c_str());

	*out = strdup(tmp.c_str());
}

void _rec_mkdir(char *fn)
{
	char *cur=strdup(fn);
	fn[strlen(fn)-1]=0;
	char *s=strrchr(fn,'/');
	if (s)
	{
		*(s+1)=0;
		_rec_mkdir(fn);
	}
	if (cur[strlen(cur)-1]=='/')
	{
		struct stat sb;
		if (lstat(cur, &sb)<0)
		{
			clog(2,"# making dir '%s'\n", cur);
			mkdir(cur, S_IRWXU);
		}
	}
	free(cur);
}

void rec_mkdir(char const *fn)
{
	char *tmp = strdup(fn);
	_rec_mkdir(tmp);
	free(tmp);
}
