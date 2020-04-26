///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

struct config_t
{
	char fn_in[65000];
	char fn_out[65000];
	int track;
	int revolution;
	int verbose;
	int gen_listing;
	int gen_export;
	int gen_log;
	int gen_maps;
	int format;
};

enum DiskFormat
{
	FMT_ANY = 0,
	FMT_AMIGA = 1,
	FMT_IBM = 2,
};

extern config_t Config;
