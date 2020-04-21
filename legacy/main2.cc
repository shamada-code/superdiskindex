

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/mman.h>
#include "Types.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

#pragma pack(1)

struct disk_track_id
{
	u8 cyl;
	u8 head;
	u8 sect;
	u8 size;
	u16 crc;
};

struct scp_header {
	u8 magic[3];
	u8 version;
	u8 disktype;
	u8 num_revs;
	u8 start_track;
	u8 end_track;
	u8 flags;
	u8 bitcellwidth;
	u8 headcfg;
	u8 resolution;
	u32 crc;

	u32 track_head_offset[168];
};

struct track_rev {
	u32 duration;
	u32 fluxcount;
	u32 data_offset;
};

struct track_header {
	u8 magic[3];
	u8 track_num;
	track_rev revs[];
};

// ---

struct _options
{
	char fn[65000];
	int track;
	int revolution;
	int verbose;
} options = { "", -1, -1, 1};

// ---

void dumpval(char const *prefix, u32 adr, u16 encval, u8 decval, int mfmerrs=0)
{
		printf("%s %08x | %04x[%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%xb]->%02x[%x%x%x%x%x%x%x%xb] | '%c' %s\n", prefix, adr, 
			encval,
			(encval>>15)&1,
			(encval>>14)&1,
			(encval>>13)&1,
			(encval>>12)&1,
			(encval>>11)&1,
			(encval>>10)&1,
			(encval>>9)&1,
			(encval>>8)&1,
			(encval>>7)&1,
			(encval>>6)&1,
			(encval>>5)&1,
			(encval>>4)&1,
			(encval>>3)&1,
			(encval>>2)&1,
			(encval>>1)&1,
			(encval>>0)&1,
			decval,
			(decval>>7)&1,
			(decval>>6)&1,
			(decval>>5)&1,
			(decval>>4)&1,
			(decval>>3)&1,
			(decval>>2)&1,
			(decval>>1)&1,
			(decval>>0)&1,
			(decval<0x20)?'.':((decval>=0x80)?'.':decval),
			mfmerrs>0?"[ERR]":"[ok]"
		);
}

inline u8 printablechar(u8 b) { return (b<0x20)?'.':((b>=0x80)?'.':b); }

void hexdump(u8 *data, u32 len)
{
	for (u32 i=0; i<(len>>4); i++)
	{
		printf("%08x  |  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  | %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c\n",
			i*0x10,
			data[i*0x10+0x0],
			data[i*0x10+0x1],
			data[i*0x10+0x2],
			data[i*0x10+0x3],
			data[i*0x10+0x4],
			data[i*0x10+0x5],
			data[i*0x10+0x6],
			data[i*0x10+0x7],
			data[i*0x10+0x8],
			data[i*0x10+0x9],
			data[i*0x10+0xa],
			data[i*0x10+0xb],
			data[i*0x10+0xc],
			data[i*0x10+0xd],
			data[i*0x10+0xe],
			data[i*0x10+0xf],
			printablechar(data[i*0x10+0x0]),
			printablechar(data[i*0x10+0x1]),
			printablechar(data[i*0x10+0x2]),
			printablechar(data[i*0x10+0x3]),
			printablechar(data[i*0x10+0x4]),
			printablechar(data[i*0x10+0x5]),
			printablechar(data[i*0x10+0x6]),
			printablechar(data[i*0x10+0x7]),
			printablechar(data[i*0x10+0x8]),
			printablechar(data[i*0x10+0x9]),
			printablechar(data[i*0x10+0xa]),
			printablechar(data[i*0x10+0xb]),
			printablechar(data[i*0x10+0xc]),
			printablechar(data[i*0x10+0xd]),
			printablechar(data[i*0x10+0xe]),
			printablechar(data[i*0x10+0xf])
			);
	}
}

// ---

class SectorMap
{
public:
	SectorMap() {}
	~SectorMap() {}

	void Set(int c, int h, int s, char state) { Map[c][h][s]=state; }

	void DumpSectorMap(int cyls, int heads, int sects)
	{
		for (int h=0; h<heads; h++)
		{
			printf("===== Head %01d =====\n",h);
			printf("     ");
			for (int c=0; c<cyls; c++)
			{
				printf("%d", c/10);
			}
			printf("\n     ");
			for (int c=0; c<cyls; c++)
			{
				printf("%d", c%10);
			}
			printf("\n     ");
			for (int c=0; c<cyls; c++)
			{
				printf("-");
			}
			printf("\n");
			for (int s=1; s<sects; s++)
			{
				printf("%02d | ", s);
				for (int c=0; c<cyls; c++)
				{
					printf("%c", Map[c][h][s]);
				}
				printf("\n");
			}
			printf("\n");
		}
	}

protected:
	char Map[90][2][40] = {'.'};
};

SectorMap SMap;

// ---

class CRC
{
public:
	CRC() { Reset(); }
	~CRC() {}

	void Reset()
	{
		ringbuf=0xffff;
	}

	void Feed(u8 db)
	{
		for (int i=0; i<8; i++) FeedBit((db>>(7-i))&0x1);
	}

	void FeedBit(u8 b)
	{
		//printf("r%04x\n", ringbuf);
		u8 f = (b^((ringbuf>>15)&1))&1;
		//printf("f%04x\n", f);
		//printf("a%04x\n", (((((ringbuf>>4)&1)^f)&1)<<5));
		//printf("b%04x\n", (((((ringbuf>>11)&1)^f)&1)<<12));
		ringbuf = ((ringbuf<<1) & 0xefde) | (((((ringbuf>>4)&1)^f)&1)<<5) | (((((ringbuf>>11)&1)^f)&1)<<12) | f;
		//printf("o%04x\n", ringbuf);
	}

	u16 Get()
	{
		return ringbuf;
	}
protected:
	u16 ringbuf;
};

// ---

template <class T, int len> class RingBuffer
{
public:
	RingBuffer() { memset(data, 0, len*sizeof(T)); memset(target, 0, len*sizeof(T)); TriggerFlag=false;}
	~RingBuffer() {}

	void SetTarget(T *tgt)
	{
		memcpy(target, tgt, len*sizeof(T));
	}

	void Feed(T val) { 
		for (int i=1; i<len; i++) {
			data[i-1]=data[i];
		}
		data[len-1]=val; 
		if (memcmp(data, target, len*sizeof(T))==0)
		{
			TriggerFlag=true;
		}
	}

	bool hasTriggered() { 
		bool retval = TriggerFlag; 
		TriggerFlag=false; 
		return retval; 
	}

protected:
	T data[len];
	T target[len];
	bool TriggerFlag;
};

// ---

class MFMDecoder
{
public:
	MFMDecoder(bool amigamode=false)
	{
		bitbuf=0;
		bitbufusage=0;
		outputcounter=0;
		syncstate=0;
		synccounter=0;
		u8 sectidmarker[4] = {0xa1,0xa1,0xa1,0xfe};
		SectIDMarker.SetTarget(sectidmarker);
		u8 sectdatmarker[4] = {0xa1,0xa1,0xa1,0xfb};
		SectDATMarker.SetTarget(sectdatmarker);
		hlstate=0;
		ScanCounter=0;
		AmigaMode = amigamode;
	}

	virtual ~MFMDecoder()
	{

	}

	static u16 mfm_encode(u8 db)
	{
		// warning: this doesn't properly handle the clock on bit 15 yet!
		u16 encval = 
			((db&0x80)<<7) | 
			((db&0x40)<<6) | 
			((db&0x20)<<5) | 
			((db&0x10)<<4) | 
			((db&0x8)<<3) | 
			((db&0x4)<<2) | 
			((db&0x2)<<1) | 
			((db&0x1)<<0);
		for (int i=0; i<7; i++)
		{
			if ((encval&(0x5<<(i*2)))==0) encval|=(1<<(i*2+1));
		}
		return encval;
	}

	void Desync()
	{
		if (options.verbose>=3) printf("Resyncing...\n");
		syncstate=0;
		synccounter=0;
		bitbuf=0;
		bitbufusage=0;
		hlstate=0;
		ScanCounter=0;
	}

	void Feed(u8 bit)
	{
		bitbuf = (bitbuf<<1)|(bit&1);
		bitbufusage++;
		if (syncstate==0)
		{
			//if (options.verbose>=3) printf("syncing: %04x (%d)\n",bitbuf,bitbufusage);
			if (AmigaMode)
			{
				if ((bitbuf&0xffffffff)==0x44894489)
				{
					// found bitclock sync
					// next wait for first decoded one bit
					bitbufusage=0;
					bitbuf=0;
					syncstate=2;
				}
			} else {
				if ((bitbuf&0xffffffff)==0xaaaaaaaa)
				{
					// found bitclock sync
					// next wait for first decoded one bit
					bitbufusage=0;
					bitbuf=0;
					syncstate=1;
				}
			}
			/*
			if (bitbufusage>=2)
			{
				if ((bitbuf&0x1)==0x0) {
					synccounter++;
					bitbuf=0;
					bitbufusage=0;
					if (synccounter>80) syncstate=1;
				} else { 
					//printf("synccount was %d.\n", synccounter);
					synccounter=0;
					bitbuf=0;
					bitbufusage=0;
				}
			}
			*/
		} else if (syncstate==1) {
			if (bitbufusage>=2)
			{
				if (bit==1) {
					// found the the first decoded 1 bit
					syncstate=2;
				} else {
					// still zero - continue looking
					bitbufusage=0;
				}
			}
		} else {
			if (bitbufusage>=16)
			{
				u16 encval = (bitbuf>>(bitbufusage-16));
				//printf("decval=%x\n",decval);
				bitbuf = bitbuf>>bitbufusage;
				bitbufusage-=16;
				if (AmigaMode)
				{
					if (AmigaWeaveBufC==0)
					{
						AmigaWeaveBuf = encval;
						AmigaWeaveBufC=1;
					} else {
						u16 odd = AmigaWeaveBuf&0x5555;
						u16 even = encval&0x5555;
						u16 decvals = (odd<<1) | (even);
						if (options.verbose>=3) dumpval("#OOBo:", 0, AmigaWeaveBuf, decvals>>8, 0);
						if (options.verbose>=3) dumpval("#OOBe:", 0, encval, decvals&0xff, 0);
						Handle((decvals>>8)&0xff);
						Handle((decvals>>0)&0xff);
						AmigaWeaveBufC=0;
					}
				} else {
					int const o=0;
					u8 decval = (
						(((encval>>(o+14))&0x1)<<7) |
						(((encval>>(o+12))&0x1)<<6) |
						(((encval>>(o+10))&0x1)<<5) |
						(((encval>>(o+8))&0x1)<<4) |
						(((encval>>(o+6))&0x1)<<3) |
						(((encval>>(o+4))&0x1)<<2) |
						(((encval>>(o+2))&0x1)<<1) |
						(((encval>>(o+0))&0x1)<<0)
					);
					Handle(decval);
				}
			}
		}
	}

	void Handle(u16 decval)
	{
		/*
		// sanity check
		int mfmerrs = 0;
		for (int i=0; i<15; i++) {
			if (((encval>>i)&0x3)==0x3) mfmerrs++;
		}
		for (int i=0; i<13; i++) {
			if (((encval>>i)&0xf)==0) mfmerrs++;
		}
		*/

		// try to compare re-encoding with original coded bits
		/*
		if ((encval&0x7fff)!=(mfm_encode(decval)&0x7fff)) { 
			mfmerrs++;
			if (options.verbose>=3) printf("[WARN] mfm coding err: %04x != %04x\n", encval, mfm_encode(decval));
			//Desync();
		}
		*/

		//dumpval("DEC:", outputcounter, encval, decval);
		outputcounter++;

		if (hlstate>0)
		{
			blockbuffer[bbidx++] = decval;
			if ( (hlstate==1) && (bbidx==6) )
			{
				CRC crc;
				crc.Feed(0xa1);
				crc.Feed(0xa1);
				crc.Feed(0xa1);
				crc.Feed(0xfe);
				crc.Feed(blockbuffer[0]);
				crc.Feed(blockbuffer[1]);
				crc.Feed(blockbuffer[2]);
				crc.Feed(blockbuffer[3]);
				crc.Feed(blockbuffer[4]);
				crc.Feed(blockbuffer[5]);
				disk_track_id *dti = (disk_track_id *)blockbuffer;
				if (options.verbose>=1) printf("*** SECTOR ID + C%02d/H%02d/S%02d + 2^(7+%d) + $%04x (%s)\n", dti->cyl, dti->head, dti->sect, dti->size, dti->crc, crc.Get()==0?"OK":"ERR");
				hlstate=0;
				bbidx=0;
				Desync();
				if (crc.Get()==0)
				{
					CurCyl=dti->cyl;
					CurHead=dti->head;
					CurSect=dti->sect;
				}
			}
			if ( (hlstate==2) && (bbidx==512+2) )
			{
				CRC crc;
				crc.Feed(0xa1);
				crc.Feed(0xa1);
				crc.Feed(0xa1);
				crc.Feed(0xfb);
				for (u32 cb=0; cb<bbidx; cb++)
				{
					crc.Feed(blockbuffer[cb]);
				}
				if ((CurCyl<0)||(CurHead<0)||(CurSect<0))
				{
					printf("[WARN] Found sector data without id!\n");
				} else {
					SMap.Set(CurCyl,CurHead,CurSect,crc.Get()==0?'O':'x');
					CurCyl=-1;
					CurHead=-1;
					CurSect=-1;
				}
				if (options.verbose>=1) printf("*** SECTOR DATA + $%04x (%s)\n", (blockbuffer[512]+(blockbuffer[513]<<8)), crc.Get()==0?"OK":"ERR");
				if (options.verbose>=2) hexdump(blockbuffer, 512);
				hlstate=0;
				bbidx=0;
				Desync();
			}
		} else {
			//if (options.verbose>=3) dumpval("#OOB:", 0, 0, decval, 0);
			ScanCounter++;
			if ((!AmigaMode)&&(ScanCounter>128)) Desync();
		}

		SectIDMarker.Feed(decval);
		if (SectIDMarker.hasTriggered()) 
		{
			//printf("Sector ID Marker found!\n"); 
			hlstate=1; 
		}
		SectDATMarker.Feed(decval);
		if (SectDATMarker.hasTriggered()) 
		{ 
			//printf("Sector DATA Marker found!\n"); 
			hlstate=2; 
		}

		// if ( (decval==0x2) || (decval==0x0) ) {
		// 	BB.Feed(0x0);
		// } else if (decval==0x1) {
		// 	BB.Feed(0x1);
		// } else {
		// 	printf("MFM Decode error!\n");
		// }
	}

	void Finish()
	{
		//BB.Finish();
		if (options.verbose>=3) printf("bytes decoded: %d\n", outputcounter);
	}

protected:
	u32 bitbuf;
	u8 bitbufusage;
	u32 outputcounter;
	int syncstate;
	int synccounter;
	//BitBuffer BB;
	RingBuffer<u8,4> SectIDMarker;
	RingBuffer<u8,4> SectDATMarker;
	int hlstate;
	u8 blockbuffer[64000];
	u32 bbidx;
	int CurCyl=-1;
	int CurHead=-1;
	int CurSect=-1;
	int ScanCounter;
	bool AmigaMode;
	u16 AmigaWeaveBuf;
	u8 AmigaWeaveBufC;
};

// ---

int main(int argc, char **argv)
{
	static_assert(sizeof(u8)==1);
	static_assert(sizeof(u16)==2);
	static_assert(sizeof(u32)==4);
	static_assert(sizeof(u64)==8);

	//char const *fn = "/storage/_moved_/Downloads/Greaseweazle-v0.11/tmp/creatures_egg_disk_2.scp";

	int ret = 0;
	while (ret>=0) {
		ret = getopt(argc, argv, "vf:t:r:");
		if (ret>=0) {
			if (ret=='f') strcpy(options.fn, optarg);
			if (ret=='t') options.track = atoi(optarg);
			if (ret=='r') options.revolution = atoi(optarg);
			if (ret=='v') options.verbose++;
		}
	}

	//printf("fn=%s\n",fn);
	//printf("t=%d\n",track);
	//printf("r=%d\n",revolution);
	//printf("v=%d\n",verbose);

	struct stat st;
	stat(options.fn, &st);

	int fd = open(options.fn, O_RDONLY);
	if (fd<0) { printf("file not found\n"); return 1; }

	int flen = st.st_size;

	u8 *fp = (u8 *)mmap(NULL, flen, PROT_READ, MAP_SHARED, fd, 0);
	if (fp==NULL) { printf("mmap failed\n"); return 1; }

	scp_header *head = (scp_header *)fp;
	//printf("%c%c%c / %02x / %d / %d-%d\n", head->magic[0], head->magic[1], head->magic[2], head->version, head->num_revs, head->start_track, head->end_track);

	//int debug_t = 0;

	int track_s = head->start_track;
	int track_e = head->end_track;
	if (options.track>=0) { track_s=options.track; track_e=options.track+1; }
	int rev_s = 0;
	int rev_e = head->num_revs;
	if (options.revolution>=0) { rev_s=options.revolution; rev_e=options.revolution+1; }

	for (int i=track_s; i<track_e; i++)
	{
		track_header *th = (track_header *)(fp+head->track_head_offset[i]);
		//printf("%c%c%c / (%03d) / %.1f ms;%10u;%10u\n", th->magic[0], th->magic[1], th->magic[2], th->track_num, th->revs[0].duration*25.0f*0.001f*0.001f, th->revs[0].fluxcount, th->revs[0].data_offset);
		for (int r=rev_s; r<rev_e; r++)
		{
			int c = th->revs[r].fluxcount;
			u16 *data = (u16 *)((u8 *)th+th->revs[r].data_offset);

			u64 thist[65536] = {0};
			for (int b=0; b<c; b++)
			{
				//if (options.verbose>=3) printf("%04x\n", data[b]);
				u16 val = ((data[b]&0xff)<<8) | ((data[b]&0xff00)>>8);
				thist[val]++;
			}
			int hs=64;
			u64 topv[hs] = {0};
			u16 topi[hs] = {0};
			for (int hi=0; hi<65536; hi++)
			{
				for (int x=0; x<hs; x++)
				{
					if (thist[hi]>=topv[x])
					{
						for (int z=(hs-2); z>=x; z--) { topv[z+1]=topv[z]; topi[z+1]=topi[z]; }
						topv[x] = thist[hi];
						topi[x] = hi;
						break;
					}
				}
			}
			for (int hi=0; hi<hs; hi++)
			{
				//printf("%.1f us (%04x); %lu\n", (float)topi[hi]*25.0f*0.001f, topi[hi], topv[hi]);
				if (options.verbose>=3) printf("%d; %lu\n", topi[hi], topv[hi]);
			}
			int n0 = 0;
			int bw = topi[n0]>>3;
			int n1 = 1;
			while ( (n1<hs) && (topi[n1]>(topi[n0]-bw)) && (topi[n1]<(topi[n0]+bw)) ) n1++;
			int n2 = n1+1;
			while ( (n2<hs) && ( ((topi[n2]>(topi[n0]-bw)) && (topi[n2]<(topi[n0]+bw))) || ((topi[n2]>(topi[n1]-bw)) && (topi[n2]<(topi[n1]+bw))) ) ) n2++;
			if (options.verbose>=3) printf("%d/%d/%d\n",n0,n1,n2);
			if (options.verbose>=3) printf("%d/%d/%d\n",topi[n0],topi[n1],topi[n2]);
			int t8ms = max( max(topi[n0], topi[n1]), topi[n2]);
			if (options.verbose>=3) printf("t_8ms = %d\n",t8ms);
			int t1 = t8ms/4;
			if (options.verbose>=3) printf("t_1 = %d\n",t1);

			MFMDecoder md(true);

			for (int b=0; b<c; b++)
			{
				u16 val = ((data[b]&0xff)<<8) | ((data[b]&0xff00)>>8);
				//printf("test: %d;%d;%d;%d (%d)\n", val, 2*t1+(t1>>1), 3*t1+(t1>>1), 4*t1+(t1>>1), t1>>1);
				if (val<(2*t1+(t1>>1))) {
					// 4us
					// feed b01
					//printf("feeding 01b (%.1fms flux)\n", (float)val*25.0f*0.001f);
					md.Feed(0);
					md.Feed(1);
				} else if (val<(3*t1+(t1>>1))) {
					// 6ms
					// feed b001
					//printf("feeding 001b (%.1fms flux)\n", (float)val*25.0f*0.001f);
					md.Feed(0);
					md.Feed(0);
					md.Feed(1);
				} else {
					// 8ms
					// feed b0001
					//printf("feeding 0001b (%.1fms flux)\n", (float)val*25.0f*0.001f);
					md.Feed(0);
					md.Feed(0);
					md.Feed(0);
					md.Feed(1);
				}
				//printf("%01x,", vq);
			}

			md.Finish();
		}
	}
	if (options.verbose>=1) SMap.DumpSectorMap(82,2,22);

	munmap(fp, flen);
	close(fd);

	return 0;
}