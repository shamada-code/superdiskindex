///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "BitStream.h"
#include "FormatDiskAmiga.h"
#include "Buffer.h"
#include "MFM.h"
#include "Helpers.h"
#include "VirtualDisk.h"
#include "DiskLayout.h"

///////////////////////////////////////////////////////////

Format::Format() : Disk(NULL),SectSize(512),DLayout(NULL),JS(NULL)
{
	DLayout = new DiskLayout();
}

Format::~Format()
{
	delete(DLayout);
}

void Format::InitLayout()
{
	DLayout->Configure(this);
}