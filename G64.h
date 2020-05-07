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

class G64
{
public:
	G64();
	virtual ~G64();

	void AddTrack(u16 track, class Buffer *rawbuffer);

	void WriteG64File();

protected:
	static const int MAXTRACKS = 84;
	static const u16 MAXTRACKLEN = 7928;

	class Buffer *Tracks[MAXTRACKS] = {0};
	u32 MaxTrackSize;
};
