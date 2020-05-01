///////////////////////////////////////////////////////////
//
// SuperDiskIndex
//
////////////////////
//
// 
//

#include "Global.h"
#include "Helpers.h"
#include "Types.h"
#include "JsonState.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <deque>

#include <stdarg.h>

///////////////////////////////////////////////////////////

typedef std::map<std::string, std::string> kvmap_t;
//typedef std::deque<std::string> json_queue_t;

///////////////////////////////////////////////////////////

std::string indent(int lvl)
{
	char const *sym="  ";
	std::string tmp;
	for (int i=0; i<lvl; i++) tmp+=sym;
	return tmp;
}

///////////////////////////////////////////////////////////

class SCtx
{
public:
	SCtx() {}
	~SCtx() {}

	void CreateFromString(std::string const &s)
	{
		col.clear();
		size_t pos = 0; 
		size_t len = 0;
		while (len<s.size())
		{
			len = s.find('.', pos)-pos;
			//printf("%d:%d(%s)\n", pos, len, s.substr(pos, len).c_str());
			col.push_back(s.substr(pos, len));
			pos+=len+1;
		}
	}

	std::string Get(int level)
	{
		if (level>=col.size()) return "";
		return col[level];
	}

	bool Compare(SCtx const &other)
	{
		if (col.size()!=other.col.size()) return false;
		for (int i=0; i<col.size(); i++) if (col[i].compare(other.col[i])!=0) return false;
		return true;
	}

	operator std::string()
	{
		std::string tmp;
		for (int i=0; i<col.size(); i++) { tmp+="/"; tmp+=col[i]; }
		return tmp;
	}

	SCtx GetPath()
	{
		SCtx gen;
		if (col.size()==0) return gen;
		for (int i=0; i<col.size()-1; i++)
		{
			gen.col.push_back(col[i]);
		}
		return gen;
	}

	std::string GetElement()
	{
		if (col.size()==0) return "";
		return col.back();
	}

	int Size() { return col.size(); }

	void Pop() { if (!col.empty()) col.pop_back(); }
	void Push(std::string s) { col.push_back(s); }

protected:
	std::deque<std::string> col;
};

///////////////////////////////////////////////////////////

struct JSStorage
{
	kvmap_t kvmap;
};

///////////////////////////////////////////////////////////

JsonState::JsonState()
{
	Storage = new JSStorage();
}

JsonState::~JsonState()
{
	delete Storage;
}

void JsonState::Test()
{
}

void JsonState::Set(char const *key, char const *valfmt, ...)
{
	va_list args;
	va_start(args, valfmt);
	size_t size = vsnprintf( nullptr, 0, valfmt, args) + 1; // Extra space for '\0'
	if( size <= 0 ) { clog(0, "ERR setting json dict value for '%s'.\n", key); return; }
	std::unique_ptr<char[]> buf( new char[ size ] ); 
	va_start(args, valfmt);
	vsnprintf( buf.get(), size, valfmt, args );
	Storage->kvmap[key] = std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

char const *JsonState::Get(char const *key)
{
	return Storage->kvmap[key].c_str();
}

void JsonState::Dump()
{
	for (kvmap_t::const_iterator it = Storage->kvmap.begin(); it!=Storage->kvmap.end(); it++)
	{
		printf("# JSON | '%s' -> '%s'.\n", it->first.c_str(), it->second.c_str());
	}
}

void JsonState::WriteToFile(char const *fn)
{
	int il=0;
	SCtx curctx,ectx;
	for (kvmap_t::const_iterator it = Storage->kvmap.begin(); it!=Storage->kvmap.end(); it++)
	{
		ectx.CreateFromString(it->first);
		//printf("%s || %s || %s\n", ((std::string)ectx).c_str(), ((std::string)ectx.GetPath()).c_str(), ((std::string)ectx.GetElement()).c_str());
		if (!ectx.GetPath().Compare(curctx))
		{
			for (int i=0; i<curctx.Size(); i++)
			{
				if ((ectx.Get(i).compare(curctx.Get(i))!=0) && (curctx.Size()>=(ectx.Size()-1)))
				{
					il--;
					printf ("%s}\n", indent(il).c_str());
					curctx.Pop();
				}
			}

			for (int i=0; i<ectx.Size()-1; i++)
			{
				if (ectx.Get(i).compare(curctx.Get(i))!=0)
				{
					printf ("%s\"%s\": {\n", indent(il).c_str(), ectx.Get(i).c_str());
					il++;
				}
			}
			curctx=ectx.GetPath();
		}
		printf("%s\"%s\": \"%s\",\n", indent(il).c_str(), ectx.GetElement().c_str(), it->second.c_str());
	}
	while (curctx.Size()>0)
	{
		il--;
		printf ("%s}\n", indent(il).c_str());
		curctx.Pop();
	}
}
