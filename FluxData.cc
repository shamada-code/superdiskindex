///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/mman.h>
#include "FluxData.h"
#include "BitStream.h"
#include "Helpers.h"

///////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////

FluxData::FluxData()
{
}

FluxData::~FluxData()
{
}

bool FluxData::Open(char const *filename)
{
	// query filesize
	struct stat st;
	stat(filename, &st);
	Size = st.st_size;

	// open file
	FD = open(filename, O_RDONLY);
	if (FD<0) { printf("file not found\n"); return false; }

	// mmap file
	Data = (u8 *)mmap(NULL, Size, PROT_READ, MAP_SHARED, FD, 0);
	if (Data==NULL) { printf("mmap failed\n"); return false; }

	// parse scp header
	scp_header *head = (scp_header *)Data;
	clog(2,"###_SCP_File_info_#################################\n");
	clog(2,"# File magic:  %c%c%c\n", head->magic[0], head->magic[1], head->magic[2]);
	clog(2,"# Version:     %02x\n", head->version);
	clog(2,"# Revolutions: %d\n", head->num_revs);
	clog(2,"# Tracks:      %d-%d\n", head->start_track, head->end_track);

	return true;
}

void FluxData::Close()
{
	munmap(Data, Size);
	close(FD);
}

int FluxData::GetTrackStart()
{
	scp_header *head = (scp_header *)Data;
	return head->start_track;
}

int FluxData::GetTrackEnd()
{
	scp_header *head = (scp_header *)Data;
	return head->end_track;
}

int FluxData::GetRevolutions()
{
	scp_header *head = (scp_header *)Data;
	return head->num_revs;
}

void FluxData::ScanTrack(int track, int rev, BitStream *bits, bool gcr_mode)
{
	/*
	int r0=0;
	int rn=GetRevolutions();
	if (Config.revolution>=0) 
	{
		r0=Config.revolution;
		rn=Config.revolution+1;
	}
	*/
	int r = rev;
	scp_header *head = (scp_header *)Data;
	track_header *th = (track_header *)(Data+head->track_head_offset[track]);
	clog(2,"###_Track_info_#################################\n");
	clog(2,"# Track:       %d\n", th->track_num);
	clog(2,"# Magic:       %c%c%c\n", th->magic[0], th->magic[1], th->magic[2]);
	clog(2,"# Rev %d Duration:    %.2fms\n", r, th->revs[r].duration*25.0f*0.001f*0.001f);
	clog(2,"# Rev %d FluxCount:   %d\n", r, th->revs[r].fluxcount);
	clog(2,"# Rev %d Offset:      0x%08x\n", r, th->revs[r].data_offset);

	//for (int r=r0; r<rn; r++)
	//{

	// first detect timing
	u32 c = th->revs[r].fluxcount;
	void *data = (u8 *)th+th->revs[r].data_offset;
	u16 t1 = DetectTimings(data, c, gcr_mode);
	
	u16 *times = (u16 *)data;
	for (u32 b=0; b<c; b++)
	{
		u8 val = Quantize(swap16(times[b]), t1);
		switch (val)
		{
			case 1:
				bits->Feed(1);
				break;
			case 2:
				bits->Feed(0);
				bits->Feed(1);
				break;
			case 3:
				bits->Feed(0);
				bits->Feed(0);
				bits->Feed(1);
				break;
			case 4:
				bits->Feed(0);
				bits->Feed(0);
				bits->Feed(0);
				bits->Feed(1);
				break;
			default:
				clog(1,"# ERR: Quantizer fail!\n");
				break;
		}
	}
	bits->Flush();
	//}
}

u16 FluxData::DetectTimings(void *data, u32 size, bool gcr_mode)
{
	u16 *dw = (u16 *)data;

	u64 thist[65536] = {0};
	for (u32 b=0; b<size; b++)
	{
		//clog(3,"%04x\n", data[b]);
		u16 val = ((dw[b]&0xff)<<8) | ((dw[b]&0xff00)>>8); // words are endian flipped in scp file
		thist[val]++;
	}
	int hs=64; // histogram size
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
		//clog(3,"%d; %lu\n", topi[hi], topv[hi]);
	}
	int n0 = 0;
	int bw = topi[n0]>>3;
	int n1 = 1;
	while ( (n1<hs) && (topi[n1]>(topi[n0]-bw)) && (topi[n1]<(topi[n0]+bw)) ) n1++;
	int n2 = n1+1;
	while ( (n2<hs) && ( ((topi[n2]>(topi[n0]-bw)) && (topi[n2]<(topi[n0]+bw))) || ((topi[n2]>(topi[n1]-bw)) && (topi[n2]<(topi[n1]+bw))) ) ) n2++;
	//clog(3,"%d/%d/%d\n",n0,n1,n2);
	int t8ms = maxval( maxval(topi[n0], topi[n1]), topi[n2]);
	int t1 = t8ms/4;
	if (gcr_mode) t1 = t8ms/3;
	clog(2,"###_Timing_info_#################################\n");
	clog(2,"# Short Seq:     %d (%.1fus)\n", topi[n0], (float)topi[n0]*25.0f*0.001f);
	clog(2,"# Med Seq:       %d (%.1fus)\n", topi[n1], (float)topi[n1]*25.0f*0.001f);
	clog(2,"# Long Seq:      %d (%.1fus)\n", topi[n2], (float)topi[n2]*25.0f*0.001f);
	clog(2,"# Bit Frequency: %d (%.1fus / %.1fkHz)\n", t1, (float)t1*25.0f*0.001f, 1000.0f/((float)t1*25.0f*0.001f));
	return t1;
}
