///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

class FluxData
{
public:
	FluxData();
	virtual ~FluxData();

	bool Open(char const *filename);
	void Close();

	int GetTrackStart();
	int GetTrackEnd();
	int GetRevolutions();

	void ScanTrack(int track, int rev, class BitStream *bits, int pass, u16 base_timing, bool gcr_mode=false);
	u16 DetectTimings(int track, int rev, bool gcr_mode);

protected:
	inline u8 Quantize(u16 td, u16 t1) 
	{ 
		if (td<((t1*1)+(t1>>1))) return 1;
		if (td<((t1*2)+(t1>>1))) return 2;
		if (td<((t1*3)+(t1>>1))) return 3;
		return 4;
	}

	float FindBestBand(u16 *dw, u32 size, float lowerlimit, float upperlimit, float lower_score, float upper_score, bool gcr_mode, int it=0);
	float CalcBandError(float band, u16 *dw, u32 size, bool gcr_mode);

	u8 *Data;
	size_t Size;
	int FD;
};
