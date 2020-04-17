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
	bool gen_listing;
	bool gen_export;
	bool gen_log;
};

extern config_t Config;
