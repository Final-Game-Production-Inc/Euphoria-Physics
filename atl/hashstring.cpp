#include "atl/hashstring.h"

#include <climits>
#include "atl/map.h"
#include "atl/string.h"
#include "file/asset.h"
#include "file/stream.h"
#include "math/amath.h"
#include "string/string.h"
#include "system/bootmgr.h"
#include "system/criticalsection.h"
#include "system/memory.h"
#include "system/param.h"
#include "system/xtl.h"

PARAM(breakonname, "Break into the debugger whenever the specified name is added to a string map");

namespace rage {

#if __WIN32
#pragma warning(disable: 4073)
#pragma init_seg(lib)
#endif

namespace atHashStringNamespaceSupport
{
	typedef atMap< u32, const char* >		RefdStringMap;
	typedef u32 (*HashFunction)(const char* str);

	template<u32 salt>	u32 SaltedAtStringHash(const char* str) { return str ? atStringHash(str, salt) : 0; }
	template<u32 salt>	u32 SaltedLiteralHash(const char* str) { return str ? atLiteralStringHash(str, salt) : 0; }

	// PURPOSE:
	//		The Namespace object holds all of the info about a particular hash string namespace, including which allocator to use,
	//		which hash function, and the map of strings.
	class Namespace {
	public:
		Namespace() {} // Empty constructor here. Make sure nothing gets re-initted if this were to run after InitNamespaces (due to static init order changes)
		sysCriticalSectionToken m_CriticalSection;
		u32 (*m_HashFunction)(const char* str);
		sysMemAllocator*		m_Allocator;
		const char*				m_StringName;
		const char*				m_ValueName;
	};


	static int g_HashStringNamespaceInitCount = 0;
	RefdStringMap*	g_StringMaps; // This is a pointer not an array to protect against initialization order problems. An array could be (re)initialized after we've already called InitNamespaces
	Namespace		g_Namespaces[HSNS_NUM_NAMESPACES] PPU_ONLY(__attribute__((init_priority(101)))) ORBIS_ONLY(__attribute__((init_priority(101))));

	EarlyInit g_InitNamespacesEarly  PPU_ONLY(__attribute__((init_priority(102)))) ORBIS_ONLY(__attribute__((init_priority(102))));

	void InitNamespace(atHashStringNamespaces nameSpace, u16 mapSize, HashFunction hashFn, sysMemAllocator* allocator, const char* stringName, const char* valueName )
	{
 		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);
		g_Namespaces[nameSpace].m_Allocator = allocator;
		if (mapSize > 0) {
			sysMemAutoUseAllocator currAlloc(allocator ? *allocator : sysMemAllocator::GetCurrent());

			g_StringMaps[nameSpace].Recompute(mapSize);
			g_StringMaps[nameSpace].SetAllowRecompute(false);
		}
		g_Namespaces[nameSpace].m_HashFunction = hashFn;
		g_Namespaces[nameSpace].m_StringName = stringName;
		g_Namespaces[nameSpace].m_ValueName = valueName;
	}

	void ShutdownNamespace(atHashStringNamespaces nameSpace)
	{
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);
		SYS_CS_SYNC(g_Namespaces[nameSpace].m_CriticalSection);
		sysMemAutoUseAllocator currAlloc(*g_Namespaces[nameSpace].m_Allocator);

		for(RefdStringMap::Iterator iter = g_StringMaps[nameSpace].CreateIterator(); !iter.AtEnd(); iter.Next())
		{
			ConstStringFree((*iter));
		}
		g_StringMaps[nameSpace].Kill();
		g_Namespaces[nameSpace].m_Allocator = NULL;
	}

	u32 ComputeHash(atHashStringNamespaces nameSpace, const char* str)
	{
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);
		return g_Namespaces[nameSpace].m_HashFunction(str);
	}

	void AddString(atHashStringNamespaces nameSpace, const u32 hash, const char* str)
	{
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);
		if ( hash == 0 )
			return;

		SYS_CS_SYNC(g_Namespaces[nameSpace].m_CriticalSection);

		const char** existingString = g_StringMaps[nameSpace].Access(hash);
		if (existingString)
		{
			// GTA V : these are horrible known collisions *SPECIFIC TO GTA V DATA* which we couldn't fix by changing the data. Yuk.
#if __ASSERT
			if (hash == 0xfdc979e8)
			{
				// a known collision between gtaV and gtaIV assets. Bummer.
				bool bKnownCollisionString = false;

				if (strcmpi(*existingString, "BM_aircon1") == 0)
				{
					bKnownCollisionString = stricmp(str,"amb@world_human_muscle_flex@arms_at_side@idle_a") == 0;
				}

				if (strcmpi(*existingString, "amb@world_human_muscle_flex@arms_at_side@idle_a") == 0)
				{
					bKnownCollisionString = stricmp(str,"BM_aircon1") == 0;
				}

				if (strcmpi(*existingString, str) == 0)
				{
					bKnownCollisionString = true;
				}

				Assertf(bKnownCollisionString, "Hash collision: strings \"%s\" and \"%s\" both hash to the same value: 0x%x in namespace %d %s", *existingString, str, hash, nameSpace, g_Namespaces[nameSpace].m_StringName);
			}
			else if (hash == 0x804db6f6)
			{
				// a known collision between gtaV and gtaIV assets. Bummer.
				bool bKnownCollisionString = false;

				if (strcmpi(*existingString, "prop_sign_sec_06+hifr") == 0)
				{
					bKnownCollisionString = stricmp(str,"hi@hei_cs1_02_1") == 0;
				}

				if (strcmpi(*existingString, "hi@hei_cs1_02_1") == 0)
				{
					bKnownCollisionString = stricmp(str,"prop_sign_sec_06+hifr") == 0;
				}

				if (strcmpi(*existingString, str) == 0)
				{
					bKnownCollisionString = true;
				}

				Assertf(bKnownCollisionString, "Hash collision: strings \"%s\" and \"%s\" both hash to the same value: 0x%x in namespace %d %s", *existingString, str, hash, nameSpace, g_Namespaces[nameSpace].m_StringName);
			}
			else  if (hash == 0x123cb7a9)
			{
				// a known collision between gtaV and gtaIV assets. Bummer.
				bool bKnownCollisionString = false;

				if (strcmpi(*existingString, "dlc_PATCHDAY17NGCRC:/content.xml") == 0)
				{
					bKnownCollisionString = stricmp(str,"VEM_SMOD4_WHLV58_t66_v3") == 0;
				}

				if (strcmpi(*existingString, "VEM_SMOD4_WHLV58_t66_v3") == 0)
				{
					bKnownCollisionString = stricmp(str,"dlc_PATCHDAY17NGCRC:/content.xml") == 0;
				}

				if (strcmpi(*existingString, str) == 0)
				{
					bKnownCollisionString = true;
				}

				Assertf(bKnownCollisionString, "Hash collision: strings \"%s\" and \"%s\" both hash to the same value: 0x%x in namespace %d %s", *existingString, str, hash, nameSpace, g_Namespaces[nameSpace].m_StringName);
			}
			else  if (hash == 0xbffebe72)
			{
				// a known collision between gtaV and gtaIV assets. Bummer.
				bool bKnownCollisionString = false;

				if (strcmpi(*existingString, "cs1_01_land_08b_shadow_proxy") == 0)
				{
					bKnownCollisionString = stricmp(str,"mp_f_freemode_01_female_heist/hand_diff_009_u_uni") == 0;
				}

				if (strcmpi(*existingString, "mp_f_freemode_01_female_heist/hand_diff_009_u_uni") == 0)
				{
					bKnownCollisionString = stricmp(str,"cs1_01_land_08b_shadow_proxy") == 0;
				}

				if (strcmpi(*existingString, str) == 0)
				{
					bKnownCollisionString = true;
				}

				Assertf(bKnownCollisionString, "Hash collision: strings \"%s\" and \"%s\" both hash to the same value: 0x%x in namespace %d %s", *existingString, str, hash, nameSpace, g_Namespaces[nameSpace].m_StringName);
			}
			else
			{
				Assertf(!stricmp(*existingString, str) , "Hash collision: strings \"%s\" and \"%s\" both hash to the same value: 0x%x in namespace %d %s", *existingString, str, hash, nameSpace, g_Namespaces[nameSpace].m_StringName);
			}
#endif //__ASSERT
		}
		else
		{
			const char* breakname=NULL;
			if (sysParam::IsInitialized() && PARAM_breakonname.Get(breakname)) // Need the IsInitiaized check because we do some string registration at static init time
			{
				if (!stricmp(breakname, str))
				{
					Warningf("Found the -breakonname string %s in namespace %d %s", str, nameSpace, g_Namespaces[nameSpace].m_StringName); // to print any diag context before the break
					__debugbreak();
				}
			}

			sysMemAutoUseAllocator currAlloc(g_Namespaces[nameSpace].m_Allocator ? *g_Namespaces[nameSpace].m_Allocator : sysMemAllocator::GetCurrent());
			g_StringMaps[nameSpace].Insert(hash, ConstStringDuplicate(str));
		}
	}

	const char* TryGetString(atHashStringNamespaces nameSpace, const u32 hash) 
	{
		if ( hash == 0 )
			return NULL;

		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);

		SYS_CS_SYNC( g_Namespaces[nameSpace].m_CriticalSection );

		const char**	refdString = g_StringMaps[nameSpace].Access( hash );
		return refdString ? *refdString : NULL;
	}

	const char* GetString(atHashStringNamespaces nameSpace, const u32 hash)
	{
		if ( hash == 0 )
			return NULL;
		
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);

		SYS_CS_SYNC( g_Namespaces[nameSpace].m_CriticalSection );

		const char**	refdString = g_StringMaps[nameSpace].Access( hash );
		Assertf( refdString, "Trying to get the string for an unreferenced hash %u, 0x%08x in namespace %d %s", hash, hash, nameSpace, g_Namespaces[nameSpace].m_StringName );
		return refdString ? *refdString : NULL;
	}

	atHashStringStats FindStats(atHashStringNamespaces nameSpace)
	{
		SYS_CS_SYNC( g_Namespaces[nameSpace].m_CriticalSection);
		
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);

		atHashStringStats	stats;
		stats.m_refdStringCount = 0;
		stats.m_refdStringChars = 0;

		RefdStringMap::Iterator		it = g_StringMaps[nameSpace].CreateIterator();
		for (it.Start(); !it.AtEnd(); it.Next())
		{
			if ( it.GetKey() == 0 )
				continue;

			const char*&		refdString = it.GetData();

			stats.m_refdStringCount += 1;
			stats.m_refdStringChars += (int)strlen(refdString);
		}

		return stats;
	}

#if __BANK
	void SaveHashStrings(atHashStringNamespaces nameSpace)
	{
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);

		SYS_CS_SYNC(g_Namespaces[nameSpace].m_CriticalSection);
		char filename[RAGE_MAX_PATH];
		formatf(filename, "hashstrings_%s.txt", g_Namespaces[nameSpace].m_StringName);
		fiStream* stream = ASSET.Create(filename, "");
		if (stream)
		{
			RefdStringMap::Iterator iter = g_StringMaps[nameSpace].CreateIterator();
			for(iter.Start(); !iter.AtEnd(); iter.Next())
			{
				fprintf(stream, "0x%08x \"", iter.GetKey());
				const char* stringPtr = iter.GetData();
				while(*stringPtr)
				{
					char16 out;
					int numBytes = Utf8ToWideChar(stringPtr, out);
					stringPtr += numBytes;

					if (numBytes == 1 && out <= 127)
					{
						switch(out)
						{
						case '"': stream->FastPutCh('\\'); stream->FastPutCh('"'); break;
						case '\n': stream->FastPutCh('\\'); stream->FastPutCh('n'); break;
						case '\t': stream->FastPutCh('\\'); stream->FastPutCh('t'); break;
						case '\\': stream->FastPutCh('\\'); stream->FastPutCh('\\'); break;
						default: stream->FastPutCh((char)out); break;
						}
					}
					else
					{
						fprintf(stream, "\\u%04x", out);
					}
				}
				fprintf(stream, "\"\n");
			}
			stream->Close();
		}
	}
#endif

#if __NO_OUTPUT
#define INITNS(nameSpace, maxSize, hashFn, allocator, stringName, valueName) InitNamespace(nameSpace, maxSize, hashFn, (!__TOOL && !__GAMETOOL) ? allocator : NULL, "", "")
#else
#define INITNS(nameSpace, maxSize, hashFn, allocator, stringName, valueName) InitNamespace(nameSpace, maxSize, hashFn, (!__TOOL && !__GAMETOOL) ? allocator : NULL, stringName, valueName)
#endif

	void InitNamespaces()
	{
		if (g_HashStringNamespaceInitCount == 0)
		{
			g_StringMaps = rage_new RefdStringMap[HSNS_NUM_NAMESPACES];

			// Make sure the debugger support is initted
			sysBootManager::StartDebuggerSupport();

			INITNS(HSNS_ATHASHSTRING,			ATL_MAP_MAX_SIZE,		SaltedAtStringHash<0>,	sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_DEBUG_VIRTUAL), "atHashString", "atHashValue");
			INITNS(HSNS_ATFINALHASHSTRING,		0,						SaltedAtStringHash<0>,	sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL), "atFinalHashString", "atHashValue");
			INITNS(HSNS_ATLITERALHASHSTRING,	0,						SaltedLiteralHash<0>,	sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL), "atLiteralHashString", "atLiteralHashValue");
			INITNS(HSNS_DIAGSTRING,				0,						SaltedLiteralHash<0x4bbcc2da>,	sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_DEBUG_VIRTUAL), "atDiagHashString", "atDiagHashValue");
			INITNS(HSNS_STATNAMESTRING,			0,						SaltedAtStringHash<0>,	sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_DEBUG_VIRTUAL), "atStatNameHash", "atStateNameValue");

			sysBootManager::SetDebugDataElement(sysBootManager::MakeFourCc('h','s','h','n'), g_StringMaps);
		}
		g_HashStringNamespaceInitCount++;
	}

#undef INITNS

	void ShutdownNamespaces()
	{
		g_HashStringNamespaceInitCount--;
		if (g_HashStringNamespaceInitCount == 0)
		{
			for(int i = 0; i < HSNS_NUM_NAMESPACES; i++)
			{
				ShutdownNamespace((atHashStringNamespaces)i);
			}
			delete [] g_StringMaps;
			g_StringMaps = NULL;
		}
	}

	const char* GetNamespaceName(atHashStringNamespaces nameSpace) { 
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);

		return g_Namespaces[nameSpace].m_StringName;
	}

	const char* GetNamespaceValueName(atHashStringNamespaces nameSpace) { 
		TrapGE(nameSpace, HSNS_NUM_NAMESPACES);

		return g_Namespaces[nameSpace].m_ValueName;
	}

#if __BANK
	void SaveAllHashStrings()
	{
		for(int i = 0; i < HSNS_NUM_NAMESPACES; i++)
		{
			SaveHashStrings((atHashStringNamespaces)i);
		}
	}
#endif


} // namespace atHashStringNamespaceSupport


} // namespace rage
