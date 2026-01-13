//
// atl/wstring.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "wstring.h"

namespace rage {

char16 null_wide_string[1];

// PURPOSE:	Append a string to the current string for char pointer
// PARAMS:	s	- const char pointer to a string to add in
//			l	- length of the adding string
// RETUNR:	VOID
void atWideString::_Append(const char* psz, int nLen)
{
	if (nLen == 0)
	{
		return;
	}

	int newlen = m_Length + nLen + 1;

	if (newlen > m_Allocated)
	{
		StringAssert(newlen <= 65535);
		char16 *ns = rage_new char16[newlen];

		// Copy only the length of the "valid" string into the new buffer
		if (m_Data)
			memcpy(ns, m_Data, m_Length << 1);

		m_Allocated = (unsigned short) newlen;

		delete[] m_Data;
		m_Data = ns;
	}

	for (int i=0; i<nLen; i++)
		m_Data[m_Length + i] = psz[i];
	m_Data[m_Length + nLen] = 0;
	m_Length = (unsigned short) (m_Length + nLen);
}

// PURPOSE:	Append a string to the current string for char16 pointer
// PARAMS:	s	- const char16 pointer to a string to add in
//			l	- length of the adding string
// RETUNR:	VOID
void atWideString::_Append(const char16* psz, int nLen)
{
	if (nLen == 0)
	{
		return;
	}

	int newlen = m_Length + nLen + 1;

	if (newlen > m_Allocated)
	{
		StringAssert(newlen <= 65535);
		char16 *ns = rage_new char16[newlen];

		// Copy only the length of the "valid" string into the new buffer
		if (m_Data)
			memcpy(ns, m_Data, m_Length << 1);

		m_Allocated = (unsigned short) newlen;

		delete[] m_Data;
		m_Data = ns;
	}

	memcpy(m_Data+m_Length,psz,nLen<<1);
	m_Data[m_Length + nLen] = 0;
	m_Length = (unsigned short) (m_Length + nLen);
}

} // namespace rage
