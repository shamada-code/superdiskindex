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
#include "TextureTGA.h"

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

void FluxData::ScanTrack(int track, int rev, BitStream *bits, int pass, bool gcr_mode)
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

	u16 w=8192; // width of fluxviz image
	u16 h=512; // height of fluxviz image
	u8 cx=2; // x (time in track) compression (LSB bits discarded)
	u8 cy=0; // y (flux timing) compression (LSB bits discarded) 
	TextureTGA *tex = NULL;
	if ((Config.gen_fluxviz)&&(pass==0))
	{
		tex=new TextureTGA(w,h);
	}

	// first detect timing
	u32 c = th->revs[r].fluxcount;
	void *data = (u8 *)th+th->revs[r].data_offset;
	u16 t1 = DetectTimings(data, c, gcr_mode);

	if ((Config.gen_fluxviz)&&(pass==0))
	{
		u8 mulmap[2][3] = {{1,2,3},{2,3,4}};
		u8 *mulsel = gcr_mode?mulmap[0]:mulmap[1];

		//int y;
		int y0,y1;
		//y=(h-1)-minval((t1*mulsel[0])>>cy, h-1); 
		y0=(h-1)-minval((t1*mulsel[0]-t1/2)>>cy, h-1); 
		y1=(h-1)-minval((t1*mulsel[0]+t1/2)>>cy, h-1); 
		tex->Rect(0,y1,w,y0-y1,0x3fff00ff);

		//y=(h-1)-minval((t1*mulsel[1])>>cy, h-1); 
		y0=(h-1)-minval((t1*mulsel[1]-t1/2)>>cy, h-1); 
		y1=(h-1)-minval((t1*mulsel[1]+t1/2)>>cy, h-1); 
		tex->Rect(0,y1,w,y0-y1,0x5fff00ff);

		//y=(h-1)-minval((t1*mulsel[2])>>cy, h-1); 
		y0=(h-1)-minval((t1*mulsel[2]-t1/2)>>cy, h-1); 
		y1=(h-1)-minval((t1*mulsel[2]+t1/2)>>cy, h-1); 
		tex->Rect(0,y1,w,y0-y1,0x7fff00ff);
	}

	u16 *times = (u16 *)data;
	for (u32 b0=0; b0<c; b0++)
	{
		u32 b;
		if (Config.reverse) b=(c-1)-b0;
		else b=b0;
		if ((Config.gen_fluxviz)&&(pass==0))
		{
			int y=(h-1)-minval(swap(times[b])>>cy, h-1);
			int x=minval((b>>cx),(u16)(w-1));
			if ((b&(0xffffffff>>(32-cx)))==0)
			{
				if (!bits->IsSynced())
				{
					tex->LineV(x,0,h,0x3fff0000);
				} else {
					switch (bits->GetActiveSyncDef())
					{
						case 0: tex->LineV(x,0,h,0x3f00ff00); break;
						case 1: tex->LineV(x,0,h,0x3f0000ff); break;
						default: tex->LineV(x,0,h,0x3fffff00);
					}
				}
			}
			tex->Pixel(x,y,0xffffffff);
		}
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

	if ((Config.gen_fluxviz)&&(pass==0))
	{
		char *fnout=NULL;
		gen_output_filename(&fnout, Config.fn_out, OT_FLUXVIZ, ".tga", OutputParams("unknown", track/2,track%2,rev,0));
		clog(1,"# Generating flux visualization '%s'.\n",fnout);
		tex->Save(fnout);
		free(fnout);
		if (tex) { delete(tex); tex=NULL; }
	}
}

u16 FluxData::DetectTimings(void *data, u32 size, bool gcr_mode)
{
	u16 *dw = (u16 *)data;
	int t1 = 0;

	if (0) // version 1 of sync detect
	{
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
		t1 = t8ms/4;
		if (gcr_mode) t1 = t8ms/3;
	}
	if (1) // version 2 of sync detect
	{
		float fcur[4]={0};
		float candidates[4]={0};
		int ticks=0;
		for (u32 b=0; b<size; b++)
		{
			int q = (4*b)/size;
			//clog(3,"%04x\n", data[b]);
			float val = (u16)swap(dw[b]); // words are endian flipped in scp file
			if ( (val-fcur[q]) > (fcur[q]*0.1) )
			{
				ticks=0;
			} else {
				ticks++;
				if (ticks>40)
				{
					if ((candidates[q]==0)||(fcur[q]<candidates[q]))
					{
						candidates[q] = fcur[q];
					}
				}
			}
			float f=0.0f;
			fcur[q] = (fcur[q]*f)+(val*(1-f));
		}
		// find best best candidate from all quadrants
		float c0 = minval(minval(candidates[0],candidates[1]),minval(candidates[2],candidates[3]));
		float c1 = maxval(maxval(candidates[0],candidates[1]),maxval(candidates[2],candidates[3]));
		float d00 = abs(candidates[0]-c0);
		float d10 = abs(candidates[1]-c0);
		float d20 = abs(candidates[2]-c0);
		float d30 = abs(candidates[3]-c0);
		float d01 = abs(candidates[0]-c1);
		float d11 = abs(candidates[1]-c1);
		float d21 = abs(candidates[2]-c1);
		float d31 = abs(candidates[3]-c1);
		float davg0 = (d00+d10+d20+d30)*0.25f;
		float davg1 = (d01+d11+d21+d31)*0.25f;
		float candidate = 0;
		if (davg0<davg1)
		{
			// the frequency bands tend to the lower candidates. so use the lower bands as reference.
			candidate = (candidates[0]+candidates[1]+candidates[2]+candidates[3]-c1)/3.0f;
		} else {
			// the frequency bands tend to the higher candidates. so use the higher bands as reference.
			candidate = (candidates[0]+candidates[1]+candidates[2]+candidates[3]-c0)/3.0f;
		}
		if (gcr_mode) {
			t1=(u16)candidate;
			clog(2,"###_Timing_info_GCR_#############################\n");
			clog(2,"# Short Seq: (..11....)  %d (%.1fus)\n", 1*t1, (float)t1*1*25.0f*0.001f);
			clog(2,"# Med Seq:   (..101...)  %d (%.1fus)\n", 2*t1, (float)t1*2*25.0f*0.001f);
			clog(2,"# Long Seq:  (..1001..)  %d (%.1fus)\n", 3*t1, (float)t1*3*25.0f*0.001f);
			clog(2,"# Bit Frequency: %d (%.1fus / %.1fkHz)\n", t1, (float)t1*25.0f*0.001f, 1000.0f/((float)t1*25.0f*0.001f));
		} else {
			t1=(u16)(candidate*0.5f);
			clog(2,"###_Timing_info_MFM_#############################\n");
			clog(2,"# Short Seq: (..101....) %d (%.1fus)\n", 2*t1, (float)t1*2*25.0f*0.001f);
			clog(2,"# Med Seq:   (..1001...) %d (%.1fus)\n", 3*t1, (float)t1*3*25.0f*0.001f);
			clog(2,"# Long Seq:  (..10001..) %d (%.1fus)\n", 4*t1, (float)t1*4*25.0f*0.001f);
			clog(2,"# Bit Frequency: %d (%.1fus / %.1fkHz)\n", t1, (float)t1*25.0f*0.001f, 1000.0f/((float)t1*25.0f*0.001f));
		}
	}
	return t1;
}
