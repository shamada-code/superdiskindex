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
	int show_listing;
	int gen_export;
	int gen_log;
	int gen_maps;
	int show_maps;
	int gen_json;
	int gen_fluxviz;
	int gen_g64;
	int format;
	int reverse;
};

enum DiskFormat
{
	FMT_AMIGA=0,
	FMT_IBM,
	FMT_1541,
	FMT_1581,
	
	FMT_COUNT,
	FMT_ANY,
};

extern config_t Config;
