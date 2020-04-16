///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// main
//

#include "Global.h"
#include "FluxData.h"
#include "BitStream.h"
#include "FormatDiskIBM.h"
#include "FormatDiskAmiga.h"
#include "VirtualDisk.h"

void print_help()
{
	printf("Syntax: superdiskindex -f <infile.scp> [more options]\n");
	printf("Options:\n");
	printf("  -f <filename.scp>    | Input filename (scp file format)\n");
	printf("  -t <tracknum>        | Only analyze track <tracknum>\n");
	printf("  -r <revnum>          | Only use revolution <revnum>\n");
	printf("  -v                   | Increase verbosity (can be applied more than once)\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	printf("SuperDiskIndex v0.00\n");
	printf("\n");

	// make sure, we're using the right data widths
	static_assert(sizeof(u8)==1);
	static_assert(sizeof(u16)==2);
	static_assert(sizeof(u32)==4);
	static_assert(sizeof(u64)==8);

	// parse cmdline options
	int ret = 0;
	while (ret>=0) {
		ret = getopt(argc, argv, "vf:t:r:h?");
		if (ret>=0) {
			if (ret=='f') strcpy(Config.fn, optarg);
			if (ret=='t') Config.track = atoi(optarg);
			if (ret=='r') Config.revolution = atoi(optarg);
			if (ret=='v') Config.verbose++;
			if ((ret=='h')||(ret=='?')) { print_help(); return 0; }
		}
	}

	// Create VirtualDisk for results
	VirtualDisk *VD = new VirtualDisk();
	VD->SetLayout(90,2,30,512); // until we have a proper detection, assume worst case values

	// Initialize FluxData
	FluxData *flux = new FluxData();
	flux->Open(Config.fn);

	int track_s = flux->GetTrackStart();
	int track_e = flux->GetTrackEnd();
	if (Config.track>=0) {
		track_s=Config.track;
		track_e=Config.track+1;
	}

	for (int t=track_s; t<track_e; t++)
	{
		// scan track
		int r0=0;
		int rn=flux->GetRevolutions();
		if (Config.revolution>=0) 
		{
			r0=Config.revolution;
			rn=Config.revolution+1;
		}
		// scan revolution
		for (int r=r0; r<rn; r++)
		{
			BitStream *bits=NULL;

			{
				// check ibm format
				if (Config.verbose>=2) printf("### Checking for IBM Format \n");
				Format *fmt = new FormatDiskIBM();
				fmt->SetVirtualDisk(VD);
				bits = new BitStream(fmt);
				bits->AddSyncWord(0x44895554, (4+2)*2);
				bits->AddSyncWord(0x44895545, (512+2)*2);

				flux->ScanTrack(t,r, bits);

				delete(fmt); fmt=NULL;
				delete(bits); bits=NULL;
			}

			{
				// check amiga format
				if (Config.verbose>=2) printf("### Checking for Amiga Format \n");
				Format *fmt = new FormatDiskAmiga();
				fmt->SetVirtualDisk(VD);
				bits = new BitStream(fmt);
				bits->AddSyncWord(0x44894489, 0x1900*2);

				flux->ScanTrack(t,r, bits);

				delete(fmt); fmt=NULL;
				delete(bits); bits=NULL;
			}
		}
	}

	//VD->ExportADF("debug.adf");

	flux->Close();
	delete(flux);

	return 0;
}
