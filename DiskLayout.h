///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include <stdlib.h>
#include "Types.h"

class DiskLayout
{
public:
	DiskLayout() : Cyls(0), Heads(0), Sects(0), VariableTrackLen(false), TrackLengths(NULL) {}
	virtual ~DiskLayout() { free(TrackLengths); }

	void SetLayout(u16 cyls, u16 heads, u16 sects, bool variabletracklen);

	u16 GetCyls();
	u16 GetHeads();
	u16 GetSectors();

	bool hasVariableTrackLen();
	void SetTrackLen(u16 track, u16 sects);
	u16 GetTrackLen(u16 track);

	u32 GetTotalSectors();

	// conversions
	u32 CHStoBLK(u16 c, u16 h, u16 s);
	void BLKtoCHS(u32 blk, u16 *c, u16 *h, u16 *s);
	u16 BLKtoC(u32 blk);
	u16 BLKtoH(u32 blk);
	u16 BLKtoS(u32 blk);

protected:
	u16 Cyls;
	u16 Heads;
	u16 Sects;
	bool VariableTrackLen;
	u16 *TrackLengths;
};
