///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include "Format.h"

class FormatDiskIBM : public Format
{
public:
	FormatDiskIBM() {}
	virtual ~FormatDiskIBM() {}

	//virtual bool Detect();
	virtual void HandleBlock(class Buffer *buffer);

protected:
};
