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
#include "Buffer.h"
#include "G64.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

struct _g64_header
{
	char sig[8];
	u8 ver;
	u8 tracks;
	u16 max_track_size;
};

G64::G64()
{
	MaxTrackSize=0;
}

G64::~G64()
{
	for (int i=0; i<MAXTRACKS; i++) if (Tracks[i]) delete(Tracks[i]);
}

void G64::AddTrack(u16 track, class Buffer *rawbuffer)
{
	if (track>=MAXTRACKS) return;
	Tracks[track] = new Buffer(*rawbuffer);
	MaxTrackSize = maxval(MaxTrackSize, rawbuffer->GetFill());
}

void G64::WriteG64File()
{
	char *fn=NULL;
	gen_output_filename(&fn, Config.fn_out, OT_DISKIMAGE, ".g64", OutputParams("raw", 0,0,0,0));
	clog(1,"# Generating g64 diskimage '%s'.\n",fn);
	int fd=open(fn, O_WRONLY|O_CREAT|O_TRUNC, DEFFILEMODE);

	// file header
	_g64_header header;
	memcpy(header.sig, "GCR-1541", 8);
	header.ver=0;
	header.tracks=MAXTRACKS;
	header.max_track_size=minval(MAXTRACKLEN,MaxTrackSize);
	write(fd, &header, sizeof(header));

	// track start offset table
	u32 trackdataoffs = sizeof(header) + MAXTRACKS*4 + MAXTRACKS*4;
	for (int i=0; i<MAXTRACKS; i++)
	{
		write(fd, &trackdataoffs, sizeof(trackdataoffs));
		trackdataoffs+=minval(MAXTRACKLEN,MaxTrackSize)+2;
	}

	// speed zone table ( for now just hardcoded to 1541 defaults)
	u32 speedzone = 0;
	for (int i=0; i<MAXTRACKS; i++)
	{
		if (i<17*2) speedzone=3;
		else if (i<24*2) speedzone=2;
		else if (i<30*2) speedzone=1;
		else speedzone=0;
		write(fd, &speedzone, sizeof(speedzone));
	}

	for (int i=0; i<MAXTRACKS; i++)
	{
		u32 trackbyteswritten = 0;
		if (Tracks[i])
		{
			u16 tracksize = minval(MAXTRACKLEN,Tracks[i]->GetFill())+2;
			//printf("%d:%d\n",i,tracksize);
			write(fd, &tracksize, sizeof(tracksize));
			trackbyteswritten+=2;
			write(fd, Tracks[i]->GetBuffer(), tracksize);
			trackbyteswritten+=tracksize;
		}
		u8 zero=0;
		for (u32 z=trackbyteswritten; z<minval(MAXTRACKLEN,MaxTrackSize)+2; z++) write(fd, &zero, 1);
	}

	close(fd);
}
