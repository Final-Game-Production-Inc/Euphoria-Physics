///
// system/param.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "param.h"
#if RSG_ORBIS
#include "platform.h"
#endif

#include "alloca.h"
#include "diag/errorcodes.h"
#include "string/string.h"

#include <stdio.h>
#include <stdlib.h>

using namespace rage;

sysParam* sysParam::sm_First;
const char *sysParam::sm_ProgName = "unknown";

int sysParam::sm_Argc;
char **sysParam::sm_Argv;
bool sysParam::sm_Initialized = false;

// The following code is here in order to read command-line arguments which
// affect the size of the heaps. This is done before the PARAM system is set
// up, and so PARAMs can't be used for this. Instead, we read the command-line
// arguments directly.
#if !__FINAL
#if __PS3 || RSG_ORBIS
# if __PS3
# include <unistd.h>
# else
# include <stdlib.h>
# endif
// FindCommandLineArg() checks to see if a command-line argument
// is present (case-insensitive).  If it is, it returns true
// and sets the ptr pointed to by ppValue to the text to the
// right of the equal sign, or NULL if no value.
bool sysParam::FindCommandLineArg(const char* s, char** ppValue /*= NULL*/)
{
	// Setup
	int argc = getargc();
	char** argv = getargv();
	int len = strlen(s);

	// Look for the argument
	for( int i = 1; i < argc; i++ )
	{
		// Ptr to this argument
		const char* parg = argv[i];
		// Do case-insensitive string compare for match
		if (strnicmp(parg, s, len) == 0)
		{
			// Advance past the matching chars
			parg += len;
			// make sure we didn't just hit a partial match on
			// a similarly-named argument,
			if ((*parg != 0) && (*parg != '=') && (*parg != ' '))
				continue;
			// Ok, we found it, let's see if there's an argument
			if (ppValue)
			{
				if (*parg == '=')
					*ppValue = (char *)(parg + 1);
				else
					*ppValue = NULL;
			}
			// Whether argument or not, we found it
			return true;
		}
	}
	// No match
	return false;
}
#elif RSG_WIN32
#include "system/xtl.h"
// FindCommandLineArg() checks to see if a command-line argument
// is present (case-insensitive).  If it is, it returns true
// and sets the ptr pointed to by ppValue to the text to the
// right of the equal sign, or NULL if no value.
bool sysParam::FindCommandLineArg(const char* s, char** ppValue /*= NULL*/)
{
	// On XENON, we just have one big long string and have to deal with that
	const char* parg = GetCommandLine();
	int len = istrlen(s);

	// Look for the argument while we still have chars
	while (*parg)
	{
		// Do case-insensitive string compare for match.
		if (strnicmp(parg, s, len) != 0)
		{
			++parg;
			continue;
		}
		// We think we have a match, but it might be partial
		const char* p1 = parg + len;
		if ((*p1 != 0) && (*p1 != '=') && (*p1 != ' '))
		{
			++parg;
			continue;
		}
		// Ok, we really have a value
		if (ppValue)
		{
			if (*p1 == '=')
				*ppValue = (char *)(p1 + 1);
			else
				*ppValue = NULL;
		}
		// Whether argument or not, we found it
		return true;
	}
	// No match
	return false;
}
#endif	// __PS3

//	GetCommandLineArgInt() looks for a command line argument with an =value
//	after it. If both are found, the integer value is returned. If not found,
//	or found but no =value, the supplied default value is returned.
int sysParam::GetCommandLineArgInt(const char *s, int defaultValue /*= 0*/)
{
	char* pValue;
	bool bFound = FindCommandLineArg(s, &pValue);
	if (bFound && (pValue != NULL))		// might be present but no =value
		return atoi(pValue);
	else
		return defaultValue;
}
//	GetCommandLineArgFloat() is the same as the int version, for float args.
float sysParam::GetCommandLineArgFloat(const char *s, float defaultValue /*= 0.0f*/)
{
	char *pValue;
	bool bFound = FindCommandLineArg(s, &pValue);
	if (bFound && (pValue != NULL))		// might be present but no =value
		return (float) atof(pValue);
	else
		return defaultValue;
}
#endif

/*
PURPOSE
	Constructor.  Use the PARAM or FPARAM macros in the header file to declare sysParam objects.
*/
sysParam::sysParam(int index,const char *name, bool required
#if !__NO_OUTPUT
				   , const char *desc
#endif
				   ) {
	m_Index = index;
	m_Name = name;
	m_Required = required;
#if !__NO_OUTPUT
	m_Desc = desc;
#endif
	m_Value = 0;
	m_Next = sm_First;
	sm_First = this;
}

#if !__NO_OUTPUT
struct sysHelpItemList {
	const char *name;
	sysParam *p;
};

static int cmpStr(const void *a,const void *b) {
	return strcmp(*(const char**)a,*(const char**)b);
}
static int cmpHelpItems(const void *a,const void *b) {
	return strcmp(((sysHelpItemList *)a)->name,((sysHelpItemList *)b)->name);
}

const char *sysParam::GetSectionName(char *dest,int destSize) {
	const char *src = m_Desc;
	if (m_Index)
		safecpy(dest," Fixed Params ",destSize);	// spaces around it cause it to sort higher :-)
	else if (src[0] == '[') {
		++src;
		while (*src) {
			if (src[0] == ']') {
				++src;
				if (src[0] == ' ') {
					++src;
				}
				break;
			}
			else if (destSize>1) {
				*dest++ = *src++;
				--destSize;
			}
			else
				src++;
		}
		*dest = 0;
	}
	else
		safecpy(dest," Global ",destSize);
	return src;
}


void sysParam::Help(const char *matchSection, bool sectionNamesOnly) {
	static bool showed_help;

	if (showed_help) {
		return;
	}
	else {
		showed_help = true;
	}

	sysParam *p;

	if (!matchSection || matchSection[0] != '=') {
		Printf("Usage: %s", sm_ProgName);

		int strings = 0;
		for (p=sm_First; p; p=p->m_Next) {
			if ((int)p->m_Index) {
				if (strings < p->m_Index) {
					strings = p->m_Index;
				}
			}
		}
		if (strings) {
			for (int i=0; i<strings; i++) {
				for (p=sm_First; p; p=p->m_Next) {
					if (i+1 == p->m_Index)
						Printf(" %s",p->m_Name);
				}
			}
		}

		for(p = sm_First; p; p = p->m_Next) {
			if (p->m_Required) {
				Printf(" -%s=[%s]", p->m_Name, p->m_Name);
			}
		}

		Printf(" [options]\nWhere [options] are one or more of:\n");
	}

	const int maxPrintedSections = 64;
	char *printedSections[maxPrintedSections];
	int itemsInSectionCount[maxPrintedSections];
	int printedSectionCount = 0;

	memset(itemsInSectionCount, 0, sizeof(int) * maxPrintedSections);

	int maxItemCount = 0;

	// Tally all unique section names
	for (p=sm_First; p; p=p->m_Next) {
		char section[64];
		p->GetSectionName(section,sizeof(section));
		int i;
		for (i=0; i<printedSectionCount; i++) {
			if (!strcmp(printedSections[i],section)) {
				break;
			}
		}
		if (i == printedSectionCount) {
			if (printedSectionCount < maxPrintedSections) {
				printedSections[printedSectionCount] = (char*)alloca(StringLength(section)+1);
				strcpy(printedSections[printedSectionCount],section);
				++printedSectionCount;
			}
		}
		itemsInSectionCount[i]++;
		maxItemCount = (itemsInSectionCount[i] > maxItemCount) ? itemsInSectionCount[i] : maxItemCount;
	}
	if (matchSection && matchSection[0] == '=') {
		printedSections[0] = (char*)matchSection+1;
		printedSectionCount = 1;
	}
	else {
		qsort(printedSections,printedSectionCount,sizeof(char*),cmpStr);
	}

	// Create the temporary array to hold the data
	sysHelpItemList *workArray = (sysHelpItemList *)alloca(sizeof(sysHelpItemList) * maxItemCount);

	if (sectionNamesOnly) {
		Printf("\n==== Available help sections ===\n");
	}

	for (int pass=0; pass<printedSectionCount; pass++) {
		if (sectionNamesOnly) {
			Printf("  %s\n", printedSections[pass]);
			continue;
		}

		Printf("\n==== [%s] ====\n",printedSections[pass]);
		// First pass through section, build the array to sort
		int idx = 0;
		char section[64];
		for (p=sm_First; p; p=p->m_Next) {
			p->GetSectionName(section,sizeof(section));
			if (strcmp(section,printedSections[pass])) {
				continue;
			}
			workArray[idx].name = p->m_Name;
			workArray[idx++].p = p;
		}
		qsort(workArray, idx, sizeof(sysHelpItemList), cmpHelpItems);

		// 2nd pass, print the sorted names
		for (int i = 0; i < idx; ++i) {
			const int indent = 10, width = 79;
			int column;

			p = workArray[i].p;
			char section[64];
			const char *cur = p->GetSectionName(section,sizeof(section));

			if (!p->m_Index) {
				Printf("-%s: ",p->m_Name);
				column = 3 + StringLength(p->m_Name);
			}
			else {
				Printf("%s: ", p->m_Name);
				column = 2 + StringLength(p->m_Name);
			}

			while (column < indent) {
				++column;
				Printf(" ");
			}
			int remain = width - column;
			while (*cur) {
				const char *start = cur;
				while (cur[1] && cur[1] != 32 && *cur != '-') {
					++cur;
				}
				// cur is now the last character we can print
				if (cur - start >= remain) {
					Printf("\n");
					column = 0;
					while (++column <= indent) {
						Printf(" ");
					}
					remain = width - indent;
				}
				while (start <= cur) {
					--remain;
					Printf("%c",*start++);
				}
				++cur;
				if (*cur == 32) {
					++cur;
					if (--remain > 1) {
						Printf(" ");
					}
				}
			}
			Printf("\n");
		}
	}
}
#endif

inline bool isParam(const char *arg) {
	// xgSubmit turns - switches into / switches to detect and allow it only in that case.
#if __WIN32PC
	if (getenv("xgJobID") && arg[0] =='/')
		return true;
#endif
	// options start with -, but don't be confused by negative numbers
	return arg[0]=='-' && (arg[1] < '0' || arg[1] > '9');
}

// Like stricmp, except it ignores trailing 's' on either parameter
// dst must be the actual parameter (possibly terminated by '='
// src is the parameter we're attempting to match
static int stricmps(const char *dst,const char *src) {
	int f = 0, l = 0;
	do {
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') ) {
			f -= ('A' - 'a');
		}
		else if (f == '=') {
			f = 0;
		}
		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') ) {
			l -= ('A' - 'a');
		}
		else if (l == '=') {
			l = 0;
		}
	} while (f && (f == l));

	// ignore a trailing 's' on either
	if ((f=='s' && (!*dst||*dst=='=') && !l) || (!f && l=='s' && (!*src||*src=='='))) {
		return 0;
	}

	return (f - l);
}

// Does any necessary adjustments to an argument value string
char* MakeArgValue(char* str)
{
	// Strip quotes from parameter values. 
	// e.g. -file "C:\some path\foo.jpg" will turn into a string containing just C:\some path\foo.jpg
	if (str && str[0] == '"')
	{
		size_t len = strlen(str);
		if (str[len-1] == '"')
		{
			str[len-1] = '\0';
			str = str + 1;
		}
	}

	return str;
}

void sysParam::Init(int argc,char **argv) {
	Assert(!sm_Initialized);
	sm_Initialized = true;

	sm_ProgName = argc && argv? argv[0] : "unknown";
	sm_Argc = argc;
	sm_Argv = argv;

#if !__NO_OUTPUT || RSG_PC	// PC supports command lines in __FINAL builds
	static char null_arg[] = "";

	bool anyArgs = false;

	for (int i=1; i<argc; i++) {
		// Fix stupid Outlook and VSI character substitutions.
		u8 dash = (u8)argv[i][0];
		if (dash == 0x96)		// Outlook does this...
			argv[i][0] = '-';
		else if (dash == 0xE2) {	// and VSI turns an em dash into a three-byte UTF-8 sequence: 0xE2 0x80 0x93
			argv[i][2] = '-';
			argv[i] += 2;
		}

#if !__NO_OUTPUT
		if (!stricmps(argv[i],"-help")) {
			Help(argv[i]+5);
			Quitf("See console output for help text.");
		}
		else 
#endif
		{
			char *arg = argv[i];
			if (!isParam(arg)) {
				if (!anyArgs) {
					for (sysParam *p=sm_First; p; p=p->m_Next) {
						if (p->m_Index == i) {
							p->m_Value = MakeArgValue(argv[i]);
						}
					}
				}
				continue;
			}

			++arg;

NEXT_FILTER:
			// Allow ! to negate the platform test.
			bool matchFlag = true;
			if (arg[0] == '!') {
				matchFlag = false;
				++arg;
			}

			bool matchedPlatform = true;

			if (arg[2]==':') {
					// Check for a specific platform
					if (arg[0]=='p'&&arg[1]=='c') {
						arg+=3;
						matchedPlatform = (__WIN32PC != 0) == matchFlag;
					}
					else if (arg[0]=='p'&&arg[1]=='3') {
						arg+=3;
						matchedPlatform = (__PPU != 0) == matchFlag;
					}
					else if (arg[0]=='x'&&arg[1]=='e') {
						arg+=3;
						matchedPlatform = (__XENON != 0) == matchFlag;
					}
					else if (arg[0]=='d'&&arg[1]=='u') {
						arg+=3;
						matchedPlatform = (RSG_DURANGO != 0) == matchFlag;
					}
					else if (arg[0]=='o'&&arg[1]=='r') {
						arg+=3;
						matchedPlatform = (RSG_ORBIS != 0) == matchFlag;
					}
					// Check for a specific configuration
					else if (arg[0]=='d'&&arg[1]=='e') {
						arg+=3;
						matchedPlatform = (__OPTIMIZED == 0) == matchFlag;
					}
					else if (arg[0]=='b'&&arg[1]=='e') {
						arg+=3;
						matchedPlatform = ((__OPTIMIZED && __DEV) != 0) == matchFlag;
					}
					else if (arg[0]=='b'&&arg[1]=='r') {
						arg+=3;
						matchedPlatform = ((__BANK) != 0) == matchFlag;
					}
					else if (arg[0]=='r'&&arg[1]=='e') {
						arg+=3;
						matchedPlatform = (__DEV == 0) == matchFlag;
					}
					else if (arg[0]=='x'&&arg[1]=='8') {
						arg+=3;
						matchedPlatform = (__64BIT == 0) == matchFlag;
					}
					else if (arg[0]=='x'&&arg[1]=='6') {
						arg+=3;
						matchedPlatform = (__64BIT != 0) == matchFlag;
					}
					else
					{
#if defined(__RGSC_DLL) && __RGSC_DLL
						// the socialclub.dll gets passed the command lines from the game, which may have modified
						// the original command line to "X-out" certain platform-specific params. Don't quitf here
						// in the socialclub.dll to prevent rage from quitting the process.
						continue;
#else
						Printf("sysParam::Init() - Unrecognized Platform parameter prefix: '%c%c:'\n",arg[0],arg[1]);
						Quitf(ERR_SYS_PARAM_PREFIX,"Unrecognized Platform parameter prefix.");
#endif
					}
			}

			if (!matchedPlatform) {
				arg[-3] = 'X';
				continue;
			}

			if (arg[0]=='!' || (arg[0]&&arg[1]&&arg[2]==':'))
				goto NEXT_FILTER;

			anyArgs = true;

			bool matched = false;
			for (sysParam *p = sm_First; p; p=p->m_Next) {
				if (!p->m_Index) {
					if (!stricmps(arg,p->m_Name)) {
						// Displayf("[%s] matched [%s]",arg,p->m_Name);
						char *equals = strchr(arg,'=');

						if (equals && equals[1] == '"')
						{
							Quitf(ERR_SYS_PARAM_FORMAT,"Quoted arguments can't use =. Use -%s \"%s...\" instead of -%s=\"%s...\"", p->m_Name, equals+2, p->m_Name, equals+2);
						}

						p->m_Value = !equals? (i+1<argc && !isParam(argv[i+1])? MakeArgValue(argv[i+1]) : null_arg) : equals + 1;
						matched = true;
					}
				}
			}
			if (!matched) {
				// Try again, testing for -no prefix that will negate an existing parameter (or a missing no prefix as well)
				// This is a separate pass to make sure no existing parameters are mismatched.
				for (sysParam *p = sm_First; p; p=p->m_Next) {
					if (!p->m_Index) {
						// -nofoo is on command line, and -foo is a recognized option.
						// In this case, pretend that -foo isn't set even if it already had been.
						if (arg[0]=='n'&&arg[1]=='o'&&!stricmps(arg+2,p->m_Name)) {
							p->m_Value = NULL;
							matched = true;
						}
						// -foo is on command line, and -nofoo is a recognized option.
						// In this case, pretend that -nofoo isn't set even if it already had been
						else if (p->m_Name[0]=='n'&&p->m_Name[1]=='o'&&!stricmps(arg,p->m_Name+2)) {
							p->m_Value = NULL;
							matched = true;
						}
					}
				}
			}
#if !__RESOURCECOMPILER && !__TOOL && (!defined(__RGSC_DLL) || !__RGSC_DLL)
			if (!matched && arg[0] != 'X') {
				// Channel TTY handling is done specially now outside the sysParam system so don't warn about those.  Likewise we've
				// agreed that all parameters accessed ONLY from script (instead of both code and script) should start with sc_ since
				// they won't have associated sysParam objects.
				if (strnicmp(arg,"sc_",3) && !strstr(arg,"_tty") && !strstr(arg,"_popup") && !strstr(arg,"_log") && !strstr(arg,"_all") && !strstr(arg,"_rag"))
					Warningf("*** Unknown option '%s' on command line (use -X%s to suppress).",arg,arg);
			}
#endif
		}
	}

#if !__NO_OUTPUT
	bool missingParam = false;
	for(sysParam* p = sm_First; p != NULL; p = p->m_Next)
	{
		if (p->m_Required && !p->m_Value) {
			Printf("Error: Missing required parameter -%s\n", p->m_Name);
			missingParam = true;
		}
	}

	if (missingParam)
	{
		sysParam::Help();
		__debugbreak();
#if __WIN32PC
		exit(1);
#else
		Quitf("One or more missing paramteers, see console output.");
#endif
	}
#endif

#if 0		// for debugging
	sysParam *p = sm_First;
	while (p) {
		if (p->m_Value) {
			Displayf("-%s: [%s]",p->m_Name,p->m_Value);
		}
		p = p->m_Next;
	}
#endif

#endif //!__NO_OUTPUT
}


bool sysParam::Get(int &v) const {
	if (m_Value && *m_Value) {
		v = strtol(m_Value,NULL,0);
		return true;
	}
	else {
		return false;
	}
}


bool sysParam::Get(u32 &v) const {
	if (m_Value && *m_Value) {
		v = strtoul(m_Value,NULL,0);
		return true;
	}
	else {
		return false;
	}
}


bool sysParam::Get(float &v) const {
	if (m_Value && *m_Value) {
		v = (float) atof(m_Value);
		return true;
	}
	else
		return false;
}


int sysParam::GetArray(const char **array,int count,char *buffer,int bufferSize) const {
	int stored = 0;
	if (m_Value && *m_Value) {
		safecpy(buffer,m_Value,bufferSize);
		char *comma = buffer - 1;
		while (comma && stored < count) {
			array[stored++] = comma+1;
			comma = strchr(comma+1,',');
			if (comma) {
				*comma = 0;
			}
		}
	}
	return stored;
}

int sysParam::GetArray(int *array,int count) const {
	int stored = 0;
	if (m_Value && *m_Value) {
		const char *comma = m_Value - 1;
		while (comma && stored < count) {
			array[stored++] = strtol(comma+1,NULL,0);
			comma = strchr(comma+1,',');
		}
	}
	return stored;
}

// <COMBINE sysParam::GetArray>
int sysParam::GetArray(float *array,int count) const {
	int stored = 0;
	if (m_Value && *m_Value) {
		const char *comma = m_Value - 1;
		while (comma && stored < count) {
			array[stored++] = (float)atof(comma+1);
			comma = strchr(comma+1,',');
		}
	}
	return stored;
}


const char *sysParam::GetByName(const char *arg,const char *defValue) {
	sysParam *i = sm_First;
	while (i && i->m_Name && stricmps(i->m_Name,arg))
		i = i->m_Next;
	if (i && i->m_Value) {
		if (i->m_Value[0] == 0)
			return defValue;
		else
			return i->m_Value;
	}
	else
		return NULL;
}


sysParam* sysParam::GetParamByName(const char *arg) {
	sysParam *i = sm_First;
	while (i && stricmps(i->m_Name,arg))
		i = i->m_Next;
	return i;
}


const char *sysParam::SearchArgArray(const char *arg) {
	for(int i=1; i<sm_Argc; i++) {
		if (sm_Argv[i][0]=='-' && !stricmps(sm_Argv[i]+1, arg)) {
			const char *e = strchr(sm_Argv[i],'=');
			if (e)
				return e+1;
			else
				return "";
		}
	}
	// No match
	return NULL;
}