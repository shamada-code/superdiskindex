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

///////////////////////////////////////////////////////////

#ifndef _TOOLVERSION
	#define _TOOLVERSION :unknown
#endif

#define TOOLVERSIONSTR1(val) #val
#define TOOLVERSIONSTR0(val) TOOLVERSIONSTR1(val)
#define TOOLVERSION TOOLVERSIONSTR0(_TOOLVERSION)

///////////////////////////////////////////////////////////

void print_help()
{
	printf("Syntax: superdiskindex -i <infile.scp> [more options]\n");
	printf("Options:\n");
	printf("  -i,--in <filename.scp>   | Input filename (scp file format)\n");
	printf("  -o,--out <basename>      | Output basename (without extension)\n");
	printf("  -t,--track <tracknum>    | Only analyze track <tracknum>\n");
	printf("  -r,--rev <revnum>        | Only use revolution <revnum>\n");
	printf("     --listing             | Generate file listing to file <basename>.lst\n");
	printf("     --export              | Generate disk image <basename>.{adf,img}\n");
	printf("     --log                 | Generate logfile <basename>.log\n");
	printf("  -a,--archive             | shortcut for --listing,--export and --log\n");
	printf("     --format-any          | Test for any known formats (default)\n");
	printf("     --format-amiga        | Test only for amiga format\n");
	printf("     --format-ibm          | Test only for ibm pc/atari format\n");
	printf("  -v                       | Increase verbosity (can be applied more than once)\n");
	printf("  -q,--quiet               | Set verbosity to minimum level\n");
	printf("\n");
}

void print_disclaimer()
{
	printf("******************************************************************\n");
	printf("*** A T T E N T I O N                                          ***\n");
	printf("******************************************************************\n");
	printf("*** This tool is still under development and is not final      ***\n");
	printf("*** and certainly not bug-free.                                ***\n");
	printf("*** Do not assume generated data (diskimages, listings, ...)   ***\n");
	printf("*** is correct, complete or without flaws.                     ***\n");
	printf("******************************************************************\n");
	printf("*** So, keep your scp files archived, because a newer version  ***\n");
	printf("*** of this tool might yield a lot better results              ***\n");
	printf("*** than this one!                                             ***\n");
	printf("******************************************************************\n");
	printf("\n");
}

void print_version()
{
	clog(1,"SuperDiskIndex v%s\n", TOOLVERSION);
	clog(1,"\n");
}

int main(int argc, char **argv)
{
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
		{"verbose", 0, NULL, 'v' },
		{"quiet", 0, NULL, 'q' },
		{"archive", 0, NULL, 'a' },
		{"format-any", 0, &Config.format, FMT_ANY },
		{"format-amiga", 0, &Config.format, FMT_AMIGA },
		{"format-ibm", 0, &Config.format, FMT_IBM },
		{0,0,0,0}
	};
	int ret = 0;
	while (ret>=0) {
		ret = getopt_long(argc, argv, "i:o:t:r:vqleh?", longopts, NULL);
		if (ret>=0) {
			if (ret=='i') strcpy(Config.fn_in, optarg);
			if (ret=='o') strcpy(Config.fn_out, optarg);
			if (ret=='t') Config.track = atoi(optarg);
			if (ret=='r') Config.revolution = atoi(optarg);
			if (ret=='v') Config.verbose++;
			if (ret=='q') Config.verbose=0;
			if (ret=='l') Config.gen_listing=true;
			if (ret=='e') Config.gen_export=true;
			if (ret=='g') Config.gen_log=true;
			if (ret=='a') { Config.gen_listing=true; Config.gen_export=true; Config.gen_log=true; }
			if ((ret=='h')||(ret=='?')) { print_help(); return 1; }
		}
	}

	clog_init();

	print_version();
	if (Config.verbose>=1) print_disclaimer();

	// check param compatability
	if (strlen(Config.fn_in)==0) { printf("No input file specified (-i).\n"); print_help(); return 1; }
	if ((Config.gen_export||Config.gen_listing||Config.gen_log)&&(strlen(Config.fn_out)==0)) { printf("Listing,Log and Export modes need output basename defined (-o).\n"); print_help(); return 1; }
	if ((Config.gen_export||Config.gen_listing)&&(Config.track>=0)) { printf("Listing and Export modes are not working with a limited track selection.\n"); print_help(); return 1; }

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
			clog(1,"######################################################################\n");
		clog(1,"##### Checking for '%s' Format \n", fmt->GetName());

		for (int pass=0; pass<2; pass++)
		{
			clog(1,"###################################\n");
			clog(1,"# Pass %d\n", pass);
			if (pass==1) 
			{
				if ((fmt->GetCyls()==1) && (fmt->GetHeads()==2) && (fmt->GetSects()==1))
				{
					// empty disk - we can skip this Pass.
					clog(1,"# No data found. Skipping pass.\n");
					continue;
				}
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
				BitStream *bits=NULL;
				bits = new BitStream(fmt, 0);
				bits->InitSyncWords();
				for (int r=r0; r<rn; r++)
				{
					bits->SetRev(r);
					flux->ScanTrack(t,r, bits);
				}
				delete(bits); bits=NULL;
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
					clog(0,"# '%s' looks like a valid %s, %s disk.\n", Config.fn_in, fmt->GetDiskTypeString(), fmt->GetDiskSubTypeString());
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
