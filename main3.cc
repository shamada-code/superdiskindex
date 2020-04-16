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

	int const FormatCount=2;
	pFormat Formats[FormatCount];
	Formats[0] = new FormatDiskIBM();
	Formats[1] = new FormatDiskAmiga();

	for (int formatidx=0; formatidx<FormatCount; formatidx++)
	{
		Format *fmt = Formats[formatidx];
		if (Config.verbose>=1) printf("##### Checking for '%s' Format \n", fmt->GetName());

		//fmt->SetVirtualDisk(VD);

		for (int pass=0; pass<2; pass++)
		{
			if (Config.verbose>=1) printf("# Pass %d\n", pass);
			if (pass==1) 
			{
				if (VD!=NULL) { delete(VD); VD=NULL; }
				VD = new VirtualDisk();
				VD->SetLayout(fmt->GetCyls(), fmt->GetHeads(), fmt->GetSects(), 512);
				fmt->SetVirtualDisk(VD);
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
					bits = new BitStream(fmt);
					bits->InitSyncWords();
					flux->ScanTrack(t,r, bits);
					delete(bits); bits=NULL;
				}
			}
			if (pass==0) 
			{
				if(Config.verbose>=1) printf("# Disk geometry detected: C%02d/H%02d/S%02d\n", fmt->GetCyls(), fmt->GetHeads(), fmt->GetSects());
			}
			if (pass==1) 
			{
				printf("\n");
				if (fmt->Analyze())
				{
					printf("# Looks like an valid disk.\n");
					//VD->ExportADF("debug.adf");
				}
			}
		
		}
	}

	flux->Close();
	delete(flux);

	return 0;
}
