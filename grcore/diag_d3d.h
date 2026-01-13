// 
// grcore/diag_d3d.h 
// 
// Copyright (C) 2012 Rockstar Games.  All Rights Reserved. 
//
#ifndef GRCORE_DIAG_D3D_H
#define GRCORE_DIAG_D3D_H

#define RSG_DXDIAG_ENABLED 1

#if __WIN32PC && RSG_DXDIAG_ENABLED

#pragma warning(push)
#pragma warning(disable : 4668)
#include <dxdiag.h>
#pragma warning(pop)
#include <dxgi.h>

#include "string/stringbuilder.h"

namespace rage
{

class DXDiag
{
	public:
		enum Category
		{
			System	= 1 << 0,
			Display	= 1 << 1,
			Sound	= 1 << 2,
			Input	= 1 << 3,

			All		= ~0
		};
		static void DumpDXDiagVerbose(char** ppOutput);
		static void DumpDXDiagByCategory(char** ppOutput, int categories);
		static void DumpDisplayableInfoString(char** ppOutput, bool multiLine);

		static wchar_t* GetCardName(DXGI_ADAPTER_DESC &oAdapterDesc);
		static void GetOsName(char* pszOS, int bufferCount);

		struct OutputMethod 
		{
			rage::atStringBuilder stringBuilder;
			void Append(const char* str);
			void AppendDecimal(int i);
			void AppendFloat(float f);
			void Append(const wchar_t* wstr);
		};

	private:
		static void DumpDXDiag_Internal(char** ppOutput, bool verbose, int categories);
		static void DumpDXDiagInfoRecursive(OutputMethod& output, IDxDiagContainer* container, unsigned int depth);
		static void DumpDXDiagInfoForCategories(OutputMethod& output, IDxDiagContainer* root, int categories);

		static void DumpDXDiagInfoForProperties(OutputMethod& output, IDxDiagContainer* container, const wchar_t** properties);
		static void DumpDXDiagInfoOfChildrenForProperties(OutputMethod& output, IDxDiagContainer* container, const char* prefix, const wchar_t** properties);
		static void DefaultOutputMethod(void** dest, const char* str);
		static void IndentDXDiagInfo(OutputMethod& output, unsigned int depth);
		static bool WritePropertyValue(OutputMethod& output, IDxDiagContainer* container, const wchar_t* property);
		static void WriteVariantToString(OutputMethod& output, VARIANT* variant);

		static const int max_buffer_size;
		static bool diag_in_progress;
};
	
}

#endif // __WIN32PC


#endif // GRCORE_DIAG_D3D_H
