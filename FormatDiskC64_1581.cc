///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "FormatDiskC64_1581.h"

///////////////////////////////////////////////////////////

char const *FormatDiskC64_1581::GetName()
{
	return "C64/1581";
}

u8 FormatDiskC64_1581::GetSyncWordCount()
{
	return 0;
}

u32 FormatDiskC64_1581::GetSyncWord(int n)
{
	return 0;
}

u32 FormatDiskC64_1581::GetSyncBlockLen(int n)
{
	return 0;
}

void FormatDiskC64_1581::HandleBlock(Buffer *buffer, int currev)
{

}

bool FormatDiskC64_1581::Analyze()
{
	return false;
}

char const *FormatDiskC64_1581::GetDiskTypeString()
{
	return "N/A (fixme)";
}

char const *FormatDiskC64_1581::GetDiskSubTypeString()
{
	return "N/A (fixme)";
}
