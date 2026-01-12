#if __WIN32PC
// 
// data/certificateverify.h
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 
#ifndef DATA_CERTIFICATEVERIFY_H
#define DATA_CERTIFICATEVERIFY_H

#pragma warning(push)
#pragma warning(disable: 4668)
#define _WINSOCKAPI_ //Prevent windows.h from including winsock.h
#include <windows.h>
#include <WinCrypt.h>
#pragma warning(pop)

#include "atl/string.h"
#include "file/limits.h"

namespace rage {

class CertificateDetails
{
public:
	CertificateDetails();

	void ParseCertificate(TCHAR* details);
	void AddTimeStamp(SYSTEMTIME time) { m_TimeStamp = time; }
	atString ParseSubject(TCHAR* input, const TCHAR* key);
	wchar_t m_fileOfCertificate[RAGE_MAX_PATH];
	SYSTEMTIME m_TimeStamp;
	atString C;
	atString S;
	atString L;
	atString O;
	atString OU;
	atString CN;
};

// Generic callback for testing any non-R* signed DLL
typedef bool (*isCertificateValidCallback)(CertificateDetails &detail);

class CertificateVerify
{
public:
	static TCHAR* GetCertificateDescription(PCCERT_CONTEXT pCertCtx);
	static bool Verify(wchar_t* fileName, isCertificateValidCallback callback);
private:
	static bool DefaultVerifySignature(CertificateDetails filename);

};

} // namespace rage

#endif // DATA_CERTIFICATEVERIFY_H

#endif // __WIN32PC