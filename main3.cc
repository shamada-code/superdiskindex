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
#include <getopt.h>
#include "Helpers.h"

void print_help()
{
	printf("Syntax: superdiskindex -i <infile.scp> [more options]\n");
	printf("Options:\n");
	printf("  -i,--in <filename.scp>   | Input filename (scp file format)\n");
	printf("  -o,--out <basename>      | Input filename (scp file format)\n");
	printf("  -t,--track <tracknum>    | Only analyze track <tracknum>\n");
	printf("  -r,--rev <revnum>        | Only use revolution <revnum>\n");
	printf("     --listing             | Generate file listing to file <basename>.lst\n");
	printf("     --export              | Generate disk image <basename>.{adf,img}\n");
	printf("     --log                 | Generate logfile <basename>.log\n");
	printf("     --format-any          | Test for any known formats (default)\n");
	printf("     --format-amiga        | Test only for amiga format\n");
	printf("     --format-ibm          | Test only for ibm pc/atari format\n");
	printf("  -v                       | Increase verbosity (can be applied more than once)\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	printf("SuperDiskIndex v0.01\n");
	printf("\n");

	// make sure, we're using the right data widths
	static_assert(sizeof(u8)==1);
	static_assert(sizeof(u16)==2);
	static_assert(sizeof(u32)==4);
	static_assert(sizeof(u64)==8);

	// parse cmdline options
	option longopts[] = {
		{"in", 1, NULL, 'i' },
		{"out", 1, NULL, 'o' },
		{"track", 1, NULL, 't' },
		{"rev", 1, NULL, 'r' },
		{"listing", 0, NULL, 'l' },
		{"export", 0, NULL, 'e' },
		{"log", 0, NULL, 'g' },
		{"help", 0, NULL, 'h' },
		{"format-any", 0, &Config.format, FMT_ANY },
		{"format-amiga", 0, &Config.format, FMT_AMIGA },
		{"format-ibm", 0, &Config.format, FMT_IBM },
		{0,0,0,0}
	};
	int ret = 0;
	while (ret>=0) {
		ret = getopt_long(argc, argv, "i:o:t:r:vleh?", longopts, NULL);
		if (ret>=0) {
			if (ret=='i') strcpy(Config.fn_in, optarg);
			if (ret=='o') strcpy(Config.fn_out, optarg);
			if (ret=='t') Config.track = atoi(optarg);
			if (ret=='r') Config.revolution = atoi(optarg);
			if (ret=='v') Config.verbose++;
			if (ret=='l') Config.gen_listing=true;
			if (ret=='e') Config.gen_export=true;
			if (ret=='g') Config.gen_log=true;
			if ((ret=='h')||(ret=='?')) { print_help(); return 1; }
		}
	}

	// check param compatability
	if (strlen(Config.fn_in)==0) { printf("No input file specified (-i).\n"); print_help(); return 1; }
	if ((Config.gen_export||Config.gen_listing||Config.gen_log)&&(strlen(Config.fn_out)==0)) { printf("Listing,Log and Export modes need output basename defined (-o).\n"); print_help(); return 1; }
	if ((Config.gen_export||Config.gen_listing)&&(Config.track>=0)) { printf("Listing and Export modes are not working with a limited track selection.\n"); print_help(); return 1; }

	clog_init();

	// Initialize FluxData
	FluxData *flux = new FluxData();
	flux->Open(Config.fn_in);

	int track_s = flux->GetTrackStart();
	int track_e = flux->GetTrackEnd();
	if (Config.track>=0) {
		track_s=Config.track;
		track_e=Config.track+1;
	}

	int FormatCount=Config.format==FMT_ANY?2:1;
	pFormat *Formats = new pFormat[FormatCount];
	int formatidx=0;
	if ((Config.format==FMT_ANY)||(Config.format==FMT_IBM))
		Formats[formatidx++] = new FormatDiskIBM();
	if ((Config.format==FMT_ANY)||(Config.format==FMT_AMIGA))
		Formats[formatidx++] = new FormatDiskAmiga();

	for (int formatidx=0; formatidx<FormatCount; formatidx++)
	{
		VirtualDisk *VD=NULL;

		Format *fmt = Formats[formatidx];
		clog(1,"##### Checking for '%s' Format \n", fmt->GetName());

		for (int pass=0; pass<2; pass++)
		{
			clog(1,"###################################\n");
			clog(1,"# Pass %d\n", pass);
			if (pass==1) 
			{
				if (VD!=NULL) { delete(VD); VD=NULL; }
				VD = new VirtualDisk();
				VD->SetLayout(fmt->GetCyls(), fmt->GetHeads(), fmt->GetSects(), flux->GetRevolutions(), 512);
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
					bits = new BitStream(fmt, r);
					bits->InitSyncWords();
					flux->ScanTrack(t,r, bits);
					delete(bits); bits=NULL;
				}
			}
			if (pass==0) 
			{
				clog(1,"# Disk geometry detected: C%02d/H%02d/S%02d\n", fmt->GetCyls(), fmt->GetHeads(), fmt->GetSects());
			}
			if (pass==1) 
			{
				clog(1,"\n");
				VD->MergeRevs();
				if (fmt->Analyze())
				{
					clog(1,"# Looks like an valid disk.\n");
					//VD->ExportADF("debug.adf");
				}
			}
		
		}
	}

	flux->Close();
	delete(flux);

	clog_exit();

	return 0;
}
