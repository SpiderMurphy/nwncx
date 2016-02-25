#ifdef _MSC_VER
#pragma once
#endif

#ifndef _NWNCONNLIB_PRECOMP_H
#define _NWNCONNLIB_PRECOMP_H

//#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <memory.h>
#include <limits.h>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <list>

namespace NWN
{
	typedef unsigned long OBJECTID;
	typedef unsigned long PLAYERID;
	typedef unsigned short ResType;

	struct ResRef16
	{
		char value[16];
	};

	struct ResRef32
	{
		char value[32];
	};

	struct Vector2
	{
		float x;
		float y;
	};

	struct Vector3 : Vector2
	{
		float z;
	};

	struct NWNCOLOR
	{
		float r, g, b, a;
	};

	struct ExoLocString
	{
		bool        IsStrRef;
		std::string String;
		bool        Flag;
		ULONG       StrRef;
	};

	struct NWN2_DataElement
	{
		std::vector< bool>            Bools;
		std::vector< int >            Ints;
		std::vector< float >          Floats;
		std::vector< unsigned long >  StrRefs;
		std::vector< std::string >    Strings;
		std::vector< ExoLocString >   LocStrings;
		std::vector< OBJECTID >       ObjectIds;
	};
}

#include "BufferBuilder.h"
#include "BufferParser.h"

//#include "../zlib/zlib.h"
//#include "../SkywingUtils/SkywingUtils.h"
//#include "../NWNBaseLib/NWNBaseLib.h"

#endif
