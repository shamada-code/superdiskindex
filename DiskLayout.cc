///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "Types.h"
#include "DiskLayout.h"
#include "Helpers.h"

void DiskLayout::SetLayout(u16 cyls, u16 heads, u16 sects, bool variabletracklen) 
{ 
	Cyls=cyls; 
	Heads=heads; 
	Sects=sects; 
	VariableTrackLen=variabletracklen;
	if (VariableTrackLen)
	{
		TrackLengths = (u16 *)realloc(TrackLengths, sizeof(*TrackLengths)*Cyls);
	} else {
		free(TrackLengths);
		TrackLengths=NULL;
	}
}

u16 DiskLayout::GetCyls() 
{ 
	return Cyls;
}

u16 DiskLayout::GetHeads() 
{ 
	return Heads; 
}

u16 DiskLayout::GetSectors() 
{ 
	return Sects; 
}

bool DiskLayout::hasVariableTrackLen() 
{ 
	return VariableTrackLen; 
}

void DiskLayout::SetTrackLen(u16 track, u16 sects)
{
	if (VariableTrackLen)
	{
		if (track>=Cyls) {
			clog(0, "# ERR: Trying to set variable tracklen on track beyond end of disk.\n");
			return;
		}
		TrackLengths[track] = sects;
	} else {
		clog(0, "# ERR: Trying to set variable tracklen on non-variable tracklen layout.\n");
	}
}

u16 DiskLayout::GetTrackLen(u16 track)
{
	if (VariableTrackLen)
	{
		if (track>=Cyls) return 0;
		return TrackLengths[track];
	}
	// else
	return Sects;
}

u32 DiskLayout::GetTotalSectors()
{
	if (VariableTrackLen)
	{
		return CHStoBLK(Cyls,0,0);
	}
	// else
	return Cyls*Heads*Sects;
}

u32 DiskLayout::CHStoBLK(u16 c, u16 h, u16 s)
{
	if (VariableTrackLen)
	{
		u32 blk = 0;
		for (int i=0; i<c; i++) blk+=GetTrackLen(i)*Heads;
		blk+=h*GetTrackLen(c);
		blk+=s;
		return blk;
	}
	return (c*Heads+h)*Sects+s;
}

void DiskLayout::BLKtoCHS(u32 blk, u16 *c, u16 *h, u16 *s)
{
	u16 _c=0;
	u16 _h=0;
	u16 _s=0;
	if (VariableTrackLen)
	{
		while (blk>=GetTrackLen(_c)*Heads) { blk-=GetTrackLen(_c)*Heads; _c++; }
		while (blk>=GetTrackLen(_c)) { blk-=GetTrackLen(_c); _h++; }
		_s = blk;
	} else {
		_c = blk/(Sects*Heads);
		blk = blk%(Sects*Heads);
		_h = blk/Sects;
		blk = blk%Sects;
		_s = blk;
	}
	if (c!=NULL) *c=_c;
	if (h!=NULL) *h=_h;
	if (s!=NULL) *s=_s;
}

u16 DiskLayout::BLKtoC(u32 blk)
{
	u16 tmp=0;
	BLKtoCHS(blk, &tmp, NULL, NULL);
	return tmp;
}

u16 DiskLayout::BLKtoH(u32 blk)
{
	u16 tmp=0;
	BLKtoCHS(blk, NULL, &tmp, NULL);
	return tmp;
}

u16 DiskLayout::BLKtoS(u32 blk)
{
	u16 tmp=0;
	BLKtoCHS(blk, NULL, NULL, &tmp);
	return tmp;
}
