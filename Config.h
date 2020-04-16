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
	char fn[65000];
	int track;
	int revolution;
	int verbose;
};

extern config_t Config;