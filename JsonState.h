///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#pragma once

#include "Types.h"

///////////////////////////////////////////////////////////

class JsonState
{
public:
	JsonState();
	virtual ~JsonState();
	
	void Test();

	void Set(char const *key, char const *valfmt, ...);
	char const *Get(char const *key);

	void Dump();
	void WriteToFile(char const *fn);

protected:
	class JSStorage *Storage;
};
