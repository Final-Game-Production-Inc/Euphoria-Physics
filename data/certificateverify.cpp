#if __WIN32PC

#include "certificateverify.h"
#include <SoftPub.h>
#include <WinCrypt.h>
#include <WinTrust.h>
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Wintrust.lib")
#include <tchar.h>

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

namespace rage
{

CertificateDetails::CertificateDetails()
{
	C = "";
	S = "";
	L = "";
	O = "";
	OU = "";
	CN = "";
	m_fileOfCertificate[0] = L'\0';
}

void CertificateDetails::ParseCertificate(TCHAR* certDetails)
{
	C = ParseSubject(certDetails, "C=");
	S = ParseSubject(certDetails, "S=");
	L = ParseSubject(certDetails, "L=");
	O = ParseSubject(certDetails, "O=");
	OU = ParseSubject(certDetails, "OU=");
	CN = ParseSubject(certDetails, "CN=");
}

atString CertificateDetails::ParseSubject(TCHAR* input, const TCHAR* key)
{
	atString outStr("");
	atString strInput;
	strInput += input;

	// If key isn't present, return empty string
	int index = strInput.IndexOf(key);
	if (index == -1) return outStr;

	// Retrieve remainder of string after key
	outStr.Set(strInput, index, strInput.length() - index);
	return outStr;
}

TCHAR* CertificateVerify::GetCertificateDescription(PCCERT_CONTEXT pCertCtx)
{
	DWORD dwStrType;
	DWORD dwCount;
	TCHAR* szSubjectRDN = NULL;

	dwStrType = CERT_X500_NAME_STR;
	dwCount = CertGetNameString(pCertCtx, CERT_NAME_RDN_TYPE, 0, &dwStrType, NULL, 0);
	if (dwCount)
	{
		szSubjectRDN = rage_new TCHAR[dwCount];
		CertGetNameString(pCertCtx, CERT_NAME_RDN_TYPE, 0, &dwStrType, szSubjectRDN, dwCount);
	}

	return szSubjectRDN;
}

bool CertificateVerify::Verify(wchar_t* fileName, isCertificateValidCallback callback)
{
	bool bError = false;

	CertificateDetails detail;
	safecpy(detail.m_fileOfCertificate, fileName);

	GUID guidAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_FILE_INFO sWintrustFileInfo;
	WINTRUST_DATA      sWintrustData;
	HRESULT            hr;

	//@@: range CERTIFICATEVERIFY_VERIFY {
	memset((void*)&sWintrustFileInfo, 0x00, sizeof(WINTRUST_FILE_INFO));
	memset((void*)&sWintrustData, 0x00, sizeof(WINTRUST_DATA));

	sWintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
	sWintrustFileInfo.pcwszFilePath = fileName;
	sWintrustFileInfo.hFile = NULL;

	sWintrustData.cbStruct            = sizeof(WINTRUST_DATA);
	sWintrustData.dwUIChoice          = WTD_UI_NONE;
	sWintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
	sWintrustData.dwUnionChoice       = WTD_CHOICE_FILE;
	sWintrustData.pFile               = &sWintrustFileInfo;
	sWintrustData.dwStateAction       = WTD_STATEACTION_VERIFY;
	//@@: location CERTIFICATEVERIFY_VERIFY_COUNT_SIGNERS
	// WinVerifyTrust checks the security certificate
	hr = WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &guidAction, &sWintrustData);

	if (TRUST_E_NOSIGNATURE == hr)
	{
		Warningf("No signature found on the file.\n");
		bError = true;
	}
	else if (TRUST_E_BAD_DIGEST == hr)
	{
		Warningf("The signature of the file is invalid\n");
		bError = true;
	}
	else if (TRUST_E_PROVIDER_UNKNOWN == hr)
	{
		Warningf("No trust provider on this machine can verify this type of files.\n");
		bError = true;
	}
	else if (S_OK != hr)
	{
		Warningf("WinVerifyTrust failed. hr = %0x", hr);
		bError = false;
	}
	else
	{
		Displayf("File signature is OK.\n");

		// retrieve the signer certificate and display its information
		CRYPT_PROVIDER_DATA const *psProvData     = NULL;
		CRYPT_PROVIDER_SGNR       *psProvSigner   = NULL;
		CRYPT_PROVIDER_CERT       *psProvCert     = NULL;
		FILETIME                   localFt;
		SYSTEMTIME                 sysTime;

		psProvData = WTHelperProvDataFromStateData(sWintrustData.hWVTStateData);
		if (psProvData)
		{
			psProvSigner = WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA)psProvData, 0 , FALSE, 0);
			if (psProvSigner)
			{
				FileTimeToLocalFileTime(&psProvSigner->sftVerifyAsOf, &localFt);
				FileTimeToSystemTime(&localFt, &sysTime);
				psProvCert = WTHelperGetProvCertFromChain(psProvSigner, 0);
				if (psProvCert)
				{
					TCHAR* szCertDesc = GetCertificateDescription(psProvCert->pCert);
					detail.ParseCertificate(szCertDesc);
					if (szCertDesc)
					{
						delete[] szCertDesc;
					}
				}
				if (psProvSigner->csCounterSigners)
				{
					FileTimeToLocalFileTime(&psProvSigner->pasCounterSigners[0].sftVerifyAsOf, &localFt);
					FileTimeToSystemTime(&localFt, &sysTime);
					detail.AddTimeStamp(sysTime);            
					psProvCert = WTHelperGetProvCertFromChain(&psProvSigner->pasCounterSigners[0], 0);
					if (psProvCert)
					{
						TCHAR* szCertDesc = GetCertificateDescription(psProvCert->pCert);
						if (szCertDesc)
						{
							delete[] szCertDesc;
						}
					}
					else
					{
						bError = true;
					}
				}
			}
		}
	}
	

	if (bError)
	{
		return false;
	}
	else
	{
		sWintrustData.dwUIChoice = WTD_UI_NONE;
		sWintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
		WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &guidAction, &sWintrustData);

		// No callback, use default signature verification
		if (callback == NULL)
		{
			return DefaultVerifySignature(detail);
		}
		// Use user-defined signature verification
		else if (callback(detail))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	//@@: } CERTIFICATEVERIFY_VERIFY

}

bool CertificateVerify::DefaultVerifySignature(CertificateDetails )
{
	return true;
}


} // namespace rage

#endif // __WIN32PC