#include "wl_def.h"
#include "id_sd.h"
#include "w_wad.h"
#include "scanner.h"
using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
// Sound Index
//
////////////////////////////////////////////////////////////////////////////////

SoundIndex::SoundIndex() : priority(50)
{
	data[0] = data[1] = data[2] = NULL;
	lump[0] = lump[1] = lump[2] = -1;
}

SoundIndex::SoundIndex(const SoundIndex &other)
{
	*this = other;
}

SoundIndex::~SoundIndex()
{
	for(unsigned int i = 0;i < 3;i++)
	{
		if(data[i] != NULL)
			delete[] data[i];
	}
}

const SoundIndex &SoundIndex::operator= (const SoundIndex &other)
{
	priority = other.priority;
	for(unsigned int i = 0;i < 3;i++)
	{
		lump[i] = other.lump[i];

		if(lump[i] != -1)
		{
			data[i] = new byte[Wads.LumpLength(lump[i])];
			memcpy(data[i], other.data[i], Wads.LumpLength(lump[i]));
		}
		else
			data[i] = NULL;
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
//
// SNDINFO
//
////////////////////////////////////////////////////////////////////////////////

SoundInformation SoundInfo;

SoundInformation::SoundInformation()
{
}

void SoundInformation::Init()
{
	printf("S_Init: Reading SNDINFO defintions.\n");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("SNDINFO", &lastLump)) != -1)
	{
		ParseSoundInformation(lump);
	}
}

void SoundInformation::ParseSoundInformation(int lumpNum)
{
	FWadLump lump = Wads.OpenLumpNum(lumpNum);
	char* data = new char[Wads.LumpLength(lumpNum)];
	lump.Read(data, Wads.LumpLength(lumpNum));

	Scanner sc(data, Wads.LumpLength(lumpNum));
	delete[] data;

	while(sc.TokensLeft() != 0)
	{
		if(!sc.GetNextString())
			sc.ScriptError("Expected logical name.\n");
		string logicalName = sc.str;

		SoundIndex idx;
		bool hasAlternatives = false;

		if(sc.CheckToken('{'))
			hasAlternatives = true;

		unsigned int i = 0;
		do
		{
			if(sc.CheckToken('}') || !sc.GetNextString())
			{
				if(i == 0)
					sc.ScriptError("Expected lump name.\n");
				else
					break;
			}

			if(sc.str.compare("NULL") == 0)
				continue;
			int sndLump = Wads.CheckNumForName(sc.str.c_str());
			if(sndLump == -1)
				continue;

			idx.lump[i] = sndLump;
			if(i == 0)
				idx.data[i] = SD_PrepareSound(sndLump);
			else
			{
				idx.data[i] = new byte[Wads.LumpLength(sndLump)];

				FWadLump soundReader = Wads.OpenLumpNum(sndLump);
				soundReader.Read(idx.data[i], Wads.LumpLength(sndLump));

				if(i == 1 || idx.lump[1] == -1)
					idx.priority = READINT16(&idx.data[i][4]);

				if(i == 2 && !sc.CheckToken('}'))
					sc.ScriptError("Expected '}'.\n");
			}
		}
		while(hasAlternatives && ++i < 3);

		if(!idx.IsNull())
			soundMap[logicalName] = idx;
	}
}

const SoundIndex &SoundInformation::operator[] (const char* logical) const
{
	map<string, SoundIndex>::const_iterator it = soundMap.find(logical);
	if(it == soundMap.end())
		return nullIndex;
	return it->second;
}