// 
// grcore/diag_d3d.cpp 
// 
// Copyright (C) 2012 Rockstar Games.  All Rights Reserved. 
//
#if __WIN32PC
#include "diag_d3d.h"
#include "device.h"
#include "grcore/adapter_d3d11.h"

#if RSG_DXDIAG_ENABLED

#include "system/new.h"

#include "string/string.h"
#include "string/unicode.h"

const int rage::DXDiag::max_buffer_size = 512 * 1024;
bool rage::DXDiag::diag_in_progress = false;

void rage::DXDiag::DumpDXDiagVerbose(char** ppOutput)
{
	DumpDXDiag_Internal(ppOutput, true, 0);
}

void rage::DXDiag::DumpDXDiagByCategory(char** ppOutput, int categories)
{
	DumpDXDiag_Internal(ppOutput, false, categories);
}

// Having (verbose && categories != 0) is unlikely; both handled in one method for less code duplication
void rage::DXDiag::DumpDXDiag_Internal(char** ppOutput, bool verbose, int categories)
{
	// Prevent recursion
	if (diag_in_progress)
		return;
	diag_in_progress = true;

	// Prepare destination
	OutputMethod output;

	// Try to init COM
	bool cleanupCOM = SUCCEEDED(CoInitialize(NULL));

	// Try to get a provider
	IDxDiagProvider* provider = NULL;
	HRESULT hr = CoCreateInstance(CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER, IID_IDxDiagProvider, (LPVOID*) &provider);

	// Check we got it
	if (AssertVerify(SUCCEEDED(hr)) && AssertVerify(provider != NULL))
	{
		// Prepare to initialise
		DXDIAG_INIT_PARAMS dxDiagInitParams = 
		{
			sizeof(DXDIAG_INIT_PARAMS),
			DXDIAG_DX9_SDK_VERSION,
			false,
			NULL
		};

		// Try to initialise it
		if (AssertVerify(SUCCEEDED(provider->Initialize(&dxDiagInitParams))))
		{
			IDxDiagContainer* root = NULL;
			if (AssertVerify(SUCCEEDED(provider->GetRootContainer(&root))))
			{
				if (categories != 0)
				{
					DumpDXDiagInfoForCategories(output, root, categories);
				}

				if (verbose)
				{
					if (categories != 0)
						output.Append("\r\n\r\nDXDiag verbose output begins:\r\n");
					else
						output.Append("DXDiag output begins:\r\n");
					DumpDXDiagInfoRecursive(output, root, 0);
					output.Append("\r\nDXDiag verbose output ended.\r\n");

					root->Release();
				}
			}
		}
		else
		{
			output.Append("Unable to obtain IDxDiagContainer! Something is probably wrong with DXDiag, or DirectX isn't correctly installed.\r\n");
		}

		// Release it when done
		provider->Release();
	}
	else
	{
		output.Append("Unable to create instance of IDxDiagProvider! Something is very wrong with either COM or your DirectX install.\r\nHRESULT was");
		output.AppendDecimal(hr);
		output.Append("\r\n");
	}

	// Cleanup COM if necessary
	if (cleanupCOM)
		CoUninitialize();

	output.Append("Log ends.");

	const size_t len = output.stringBuilder.Length();
	*ppOutput = rage_new char[len+1];
	memcpy(*ppOutput, output.stringBuilder.ToString(), len);
	(*ppOutput)[len] = 0; // NULL terminator

	diag_in_progress = false;
}

void rage::DXDiag::DumpDisplayableInfoString(char** ppOutput, bool multiLine)
{
	// Prepare destination
	OutputMethod output;

	// Try to init COM
	bool cleanupCOM = SUCCEEDED(CoInitialize(NULL));

	// Try to get a provider
	IDxDiagProvider* provider = NULL;
	HRESULT hr = CoCreateInstance(CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER, IID_IDxDiagProvider, (LPVOID*) &provider);

	// Check we got it
	if (AssertVerify(SUCCEEDED(hr)) && AssertVerify(provider != NULL))
	{
		// Prepare to initialise
		DXDIAG_INIT_PARAMS dxDiagInitParams = 
		{
			sizeof(DXDIAG_INIT_PARAMS),
			DXDIAG_DX9_SDK_VERSION,
			false,
			NULL
		};

		// Try to initialise it
		if (AssertVerify(SUCCEEDED(provider->Initialize(&dxDiagInitParams))))
		{
			IDxDiagContainer* root = NULL;
			if (AssertVerify(SUCCEEDED(provider->GetRootContainer(&root))))
			{
				IDxDiagContainer* child = NULL;

				// Grab the info we need
				if (AssertVerify(SUCCEEDED(root->GetChildContainer(L"DxDiag_SystemInfo", &child))))
				{
					//static const wchar_t* props[] = { L"szProcessorEnglish", L"szPhysicalMemoryEnglish", L"szOSExEnglish", L"szDirectXVersionLongEnglish", L"dwDirectXVersionMajor", L"dwDirectXVersionMinor", L"szDirectXVersionLetter", L"szTimeEnglish", NULL };

					WritePropertyValue(output, child, L"szOSExEnglish");
					if (multiLine)
						output.Append("\n");
					else
						output.Append(", ");

#if !__TOOL
					//WritePropertyValue(output, child, L"szDirectXVersionLongEnglish");
					output.Append("DX Feature Level: ");
					u32 featureLevel = GRCDEVICE.GetDxFeatureLevel();
					output.AppendDecimal(featureLevel / 100);
					output.Append(".");
					output.AppendDecimal((featureLevel % 100) / 10);
					if (featureLevel % 10 > 0)
						output.AppendDecimal(featureLevel % 10);

					if (!multiLine)
						output.Append(", ");
#endif
					if (multiLine)
						output.Append("\n");

					WritePropertyValue(output, child, L"szProcessorEnglish");
					if (multiLine)
						output.Append("\n");
					else
						output.Append(", ");

					WritePropertyValue(output, child, L"szPhysicalMemoryEnglish");
					output.Append("\n");
					
					child->Release();
				}
				
#if __D3D11 && !__TOOL
				{
					if (!multiLine)
					{
						output.AppendDecimal(GRCDEVICE.GetWidth());
						output.Append("x");
						output.AppendDecimal(GRCDEVICE.GetHeight());
						output.Append(", ");
					}

					DXGI_ADAPTER_DESC oAdapterDesc;
					GRCDEVICE.GetAdapterDescription(oAdapterDesc);

					output.Append(GetCardName(oAdapterDesc));
					output.Append(", ");
					output.AppendDecimal(int(oAdapterDesc.DedicatedVideoMemory / (1000 * 1000)));
					output.Append("MB, Driver Version ");
					output.Append(((grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(GRCDEVICE.GetAdapterOrdinal()))->GetDriverString());
					output.Append("\n");
				}
#else
				{
					if (AssertVerify(SUCCEEDED(root->GetChildContainer(L"DxDiag_DisplayDevices", &child))))
					{
						static const wchar_t* props[] = { L"szDescription", L"szKeyDeviceID", L"szDisplayMemoryEnglish", L"szDisplayModeEnglish", L"szDriverVersion", L"szDriverDateEnglish", L"szMonitorName", NULL };

						WCHAR wname[256];
						wname[0] = (WCHAR)0;

						if (AssertVerify(SUCCEEDED(child->EnumChildContainerNames(0, (LPWSTR)wname, 256))))
						{
							IDxDiagContainer* display;
							if (AssertVerify(SUCCEEDED(child->GetChildContainer((LPWSTR)wname, &display))))
							{
								WritePropertyValue(output, display, L"szDescription");
								output.Append(", ");
								WritePropertyValue(output, display, L"szDisplayMemoryEnglish");
								output.AppendDecimal((int)GRCDEVICE.GetAvailableVideoMemory());
								output.Append("\n");

								display->Release();
							}
						}

						child->Release();
					}
				}
#endif //__D3D11
				root->Release();
			}
		}

		// Release it when done
		provider->Release();
	}

	// Cleanup COM if necessary
	if (cleanupCOM)
		CoUninitialize();

	const size_t len = output.stringBuilder.Length();
	*ppOutput = rage_new char[len+1];
	memcpy(*ppOutput, output.stringBuilder.ToString(), len);
	(*ppOutput)[len] = 0; // NULL terminator

	diag_in_progress = false;
}

#define DEBUG_DXDIAG 0

void rage::DXDiag::DumpDXDiagInfoRecursive(OutputMethod& output, IDxDiagContainer* container, unsigned int depth)
{
	// Check we've got a container
	if (!AssertVerify(container))
		return;

	DWORD count;
	static WCHAR wname[256];
	static char name[256];
	VARIANT variant;
	VariantInit(&variant);

	const unsigned int maxDepth = 20;
	unsigned int maxItems = 100;//(maxDepth + 5) - depth / 2;

	// Get property count
	if (!AssertVerify(SUCCEEDED(container->GetNumberOfProps(&count))))
		return;

	// DEBUG - something funky is going on
#if DEBUG_DXDIAG
	FILE* file = fopen("debugdxdiag.txt", "a");
#endif

	// Iterate through properties, displaying in format:
	// "<propname>: <value>\n"
	for (DWORD i = 0; i < count && i < maxItems; i++)
	{
		wname[0] = (WCHAR)0;
		if (!AssertVerify(SUCCEEDED(container->EnumPropNames(i, (LPWSTR)wname, 256))))
			continue;

		if (!AssertVerify(SUCCEEDED(container->GetProp((LPWSTR)wname, &variant))))
			continue;

		IndentDXDiagInfo(output, depth);

		wcstombs(name, wname, 256 * sizeof(char));

#if DEBUG_DXDIAG
		if (file)
		{
			fputs(name, file);
			fputs("\r\n", file);
		}
#endif


		output.Append(name);
		output.Append(" = ");

		WriteVariantToString(output, &variant);

		VariantClear(&variant);

		output.Append("\r\n");
	}

#if DEBUG_DXDIAG
	if (file)
		fclose(file);
#endif


	// Get child count
	if (!AssertVerify(SUCCEEDED(container->GetNumberOfChildContainers(&count))))
		return;

	// Writes child name
	for (DWORD i = 0; i < count && i < maxItems; i++)
	{
		wname[0] = (WCHAR)0;

		if (!AssertVerify(SUCCEEDED(container->EnumChildContainerNames(i, (LPWSTR)wname, 256))))
			continue;

		IndentDXDiagInfo(output, depth);

		wcstombs(name, wname, 256 * sizeof(char));

#if DEBUG_DXDIAG
		FILE* file = fopen("debugdxdiag.txt", "a");
		if (file)
		{
			fputs("-", file);
			fputs(name, file);
			fputs(":\r\n", file);
			fclose(file);
		}
#endif


		output.Append(name);
		output.Append(":\r\n");

		IDxDiagContainer* child;
		if (!AssertVerify(SUCCEEDED(container->GetChildContainer((LPWSTR)wname, &child))))
			continue;

		if (depth / 2 < maxDepth)
			DumpDXDiagInfoRecursive(output, child, depth + 2);

		child->Release();
	}

#if DEBUG_DXDIAG
	file = fopen("debugdxdiag.txt", "a");
	if (file)
	{
		fputs("\r\n", file);
		fclose(file);
	}
#endif
}

void rage::DXDiag::DumpDXDiagInfoForCategories(OutputMethod& output, IDxDiagContainer* container, int categories)
{
	IDxDiagContainer* child;

	if (categories & System)
	{
		output.Append("-----------\r\n");
		output.Append("System Info\r\n");
		output.Append("-----------\r\n");
		if (AssertVerify(SUCCEEDED(container->GetChildContainer(L"DxDiag_SystemInfo", &child))))
		{
			static const wchar_t* props[] = { L"szProcessorEnglish", L"szPhysicalMemoryEnglish", L"szOSExEnglish", L"szDirectXVersionLongEnglish", L"dwDirectXVersionMajor", L"dwDirectXVersionMinor", L"szDirectXVersionLetter", L"szTimeEnglish", NULL };
			DumpDXDiagInfoForProperties(output, child, props);
			child->Release();
		}
		else
			output.Append("Failed to get info!\r\n");
		output.Append("\r\n\r\n");
	}
	if (categories & Display)
	{
		output.Append("------------\r\n");
		output.Append("Display Info\r\n");
		output.Append("------------\r\n");
		if (AssertVerify(SUCCEEDED(container->GetChildContainer(L"DxDiag_DisplayDevices", &child))))
		{
			static const wchar_t* props[] = { L"szDescription", L"szKeyDeviceID", L"szDisplayMemoryEnglish", L"szDisplayModeEnglish", L"szDriverVersion", L"szDriverDateEnglish", L"szMonitorName", NULL };
			DumpDXDiagInfoOfChildrenForProperties(output, child, "Device", props);
			child->Release();
		}
		else
			output.Append("Failed to get info!\r\n");
		output.Append("\r\n\r\n");
	}
	if (categories & Sound)
	{
		if (AssertVerify(SUCCEEDED(container->GetChildContainer(L"DxDiag_DirectSound", &child))))
		{
			IDxDiagContainer* grandchild;

			output.Append("-----------------\r\n");
			output.Append("Sound Device Info\r\n");
			output.Append("-----------------\r\n");
			if (AssertVerify(SUCCEEDED(child->GetChildContainer(L"DxDiag_SoundDevices", &grandchild))))
			{
				static const wchar_t* props[] = { L"szDescription", L"szHardwareID", L"szDriverPath", L"szDriverVersion", L"szProvider", NULL };
				DumpDXDiagInfoOfChildrenForProperties(output, grandchild, "Device", props);
				grandchild->Release();
			}
			else
				output.Append("Failed to get info!");
			output.Append("\r\n\r\n");

			output.Append("-------------------------\r\n");
			output.Append("Sound Capture Device Info\r\n");
			output.Append("-------------------------\r\n");
			if (AssertVerify(SUCCEEDED(child->GetChildContainer(L"DxDiag_SoundDevices", &grandchild))))
			{
				static const wchar_t* props[] = { L"szDescription", L"szGuidDeviceID", L"szDriverPath", L"szDriverVersion", NULL };
				DumpDXDiagInfoOfChildrenForProperties(output, grandchild, "Device", props);
				grandchild->Release();
			}
			else
				output.Append("Failed to get info!\r\n");
			output.Append("\r\n\r\n");

			child->Release();
		}
		else
			output.Append("Failed to get DirectSound info!\r\n\r\n\r\n");
	}
	if (categories & Input)
	{
		if (AssertVerify(SUCCEEDED(container->GetChildContainer(L"DxDiag_DirectInput", &child))))
		{
			IDxDiagContainer* grandchild;

			output.Append("-----------------\r\n");
			output.Append("Input Device Info\r\n");
			output.Append("-----------------\r\n");
			if (AssertVerify(SUCCEEDED(child->GetChildContainer(L"DxDiag_DirectInputDevices", &grandchild))))
			{
				static const wchar_t* props[] = { L"szInstanceName", L"bAttached", L"dwVendorID", L"dwProductID", L"dwDevType", NULL };
				DumpDXDiagInfoOfChildrenForProperties(output, grandchild, "Device", props);
				grandchild->Release();
			}
			else
				output.Append("Failed to get info!\r\n");
			output.Append("\r\n\r\n");

			output.Append("--------------------\r\n");
			output.Append("Gameport Device Info\r\n");
			output.Append("--------------------\r\n");
			if (AssertVerify(SUCCEEDED(child->GetChildContainer(L"DxDiag_DirectInputGameports", &grandchild))))
			{
				static const wchar_t* props[] = { L"szDescription", L"szInstanceName", L"bAttached", L"dwVendorID", L"dwProductID", L"dwDevType", NULL };
				DumpDXDiagInfoOfChildrenForProperties(output, grandchild, "Device", props);
				grandchild->Release();
			}
			else
				output.Append("Failed to get info!\r\n");
			output.Append("\r\n\r\n");

			output.Append("---------------\r\n");
			output.Append("USB Device Info\r\n");
			output.Append("---------------\r\n");
			if (AssertVerify(SUCCEEDED(child->GetChildContainer(L"DxDiag_DirectInputUSBRoot", &grandchild))))
			{
				static const wchar_t* props[] = { L"szDescription", L"dwVendorID", L"dwProductID", L"szMatchingDeviceId", L"szService", NULL };
				DumpDXDiagInfoOfChildrenForProperties(output, grandchild, "Device", props);
				grandchild->Release();
			}
			else
				output.Append("Failed to get info!\r\n");
			output.Append("\r\n\r\n");

			output.Append("---------------\r\n");
			output.Append("PS2 Device Info\r\n");
			output.Append("---------------\r\n");
			if (AssertVerify(SUCCEEDED(child->GetChildContainer(L"DxDiag_DirectInputPS2Devices", &grandchild))))
			{
				static const wchar_t* props[] = { L"szDescription", L"dwVendorID", L"dwProductID", L"szMatchingDeviceId", L"szService", NULL };
				DumpDXDiagInfoOfChildrenForProperties(output, grandchild, "Device", props);
				grandchild->Release();
			}
			else
				output.Append("Failed to get info!\r\n");
			output.Append("\r\n\r\n");

			child->Release();
		}
	}
}

bool rage::DXDiag::WritePropertyValue(OutputMethod& output, IDxDiagContainer* container, const wchar_t* prop)
{
	VARIANT variant;
	VariantInit(&variant);

	if (!AssertVerify(SUCCEEDED(container->GetProp((LPWSTR)prop, &variant))))
		return false;

	WriteVariantToString(output, &variant);

	VariantClear(&variant);

	return true;
}

void rage::DXDiag::DumpDXDiagInfoForProperties(OutputMethod& output, IDxDiagContainer* container, const wchar_t** properties)
{
	// Check we've got a container
	if (!AssertVerify(container))
		return;

	DWORD count;
	static WCHAR wname[256];
	static char name[256];
	VARIANT variant;
	VariantInit(&variant);

	// Get property count
	if (!AssertVerify(SUCCEEDED(container->GetNumberOfProps(&count))))
		return;

	// Iterate through properties, displaying in format:
	// "<propname>: <value>\n"
	for (int i = 0; properties[i] != NULL; i++)
	{
		if (!AssertVerify(SUCCEEDED(container->GetProp((LPWSTR)properties[i], &variant))))
			continue;

		wcstombs(name, properties[i], 256 * sizeof(char));


		int tabulation = 27 - istrlen(name);
		if (tabulation < 0)
			tabulation = 0;
		IndentDXDiagInfo(output, tabulation);

		output.Append(name);
		output.Append(" = ");

		WriteVariantToString(output, &variant);

		VariantClear(&variant);

		output.Append("\r\n");
	}
}

void rage::DXDiag::DumpDXDiagInfoOfChildrenForProperties(OutputMethod& output, IDxDiagContainer* container, const char* prefix, const wchar_t** properties)
{
	DWORD count;
	static WCHAR wname[256];
	static char name[256];

	// Get child count
	if (!AssertVerify(SUCCEEDED(container->GetNumberOfChildContainers(&count))))
		return;

	// Writes child name
	for (DWORD i = 0; i < count; i++)
	{
		wname[0] = (WCHAR)0;

		if (!AssertVerify(SUCCEEDED(container->EnumChildContainerNames(i, (LPWSTR)wname, 256))))
			continue;

		IndentDXDiagInfo(output, 2);

		wcstombs(name, wname, 256 * sizeof(char));

		if (prefix)
		{
			output.Append(prefix);
			output.Append(" ");
		}
		output.Append(name);
		output.Append(":\r\n");

		IDxDiagContainer* child;
		if (!AssertVerify(SUCCEEDED(container->GetChildContainer((LPWSTR)wname, &child))))
			continue;

		DumpDXDiagInfoForProperties(output, child, properties);

		child->Release();
	}
}

void rage::DXDiag::OutputMethod::Append(const char* str)
{
	if (!AssertVerify(str != NULL))
		return;
	this->stringBuilder.Append(str);
}

void rage::DXDiag::OutputMethod::AppendDecimal(int i)
{
	char buffer[11];
	formatf(buffer, "%d", i);
	Append(buffer);
}

void rage::DXDiag::OutputMethod::AppendFloat(float f)
{
	char buffer[20];
	formatf(buffer, "%f", f);
	Append(buffer);
}

void rage::DXDiag::OutputMethod::Append(const wchar_t* wstr)
{
	if (!AssertVerify(wstr != NULL))
	{
		Append("(NULL string)");
		return;
	}
	const int max_str_len = 256;
	if (!AssertVerify(wcslen(wstr) < max_str_len))
	{
		Append("(string too long)");
		return;
	}

	char str[max_str_len*2+1] = {0};
	size_t size = wcstombs(str, wstr, max_str_len);
	str[size+1] = (char)0;

	if (size > 0)
	{
		if (strlen(str) > size)
			Append("(string size mismatch)");
		else if (size < max_str_len)
		{
			bool kosher = true;
			for (int i = 0; i < (signed)size; i++)
			{
				char c = str[i];
				if ((c < 32 || c >= 127) && c != '\n' && c != '\r')
				{
					kosher = false;
					Append("Bad char: ");
					AppendDecimal((int)c);
				}
			}
			if (kosher)
				Append(str);
		}
		else
			Append("(oversize string)");
	}
	else
	Append("(zero-length string)");
}

void rage::DXDiag::IndentDXDiagInfo(OutputMethod& output, unsigned int depth)
{
	while (depth--)
		output.Append(" ");
}

void rage::DXDiag::WriteVariantToString(OutputMethod& output, VARIANT* variant)
{
	switch (variant->vt)
	{
	case VT_EMPTY:
	case VT_NULL:
		AssertMsg(false, "Missing variable type in DXDiag info!");
		break;
	case VT_I2:
		output.AppendDecimal((int)variant->iVal);
		break;
	case VT_I4:
		output.AppendDecimal((int)variant->lVal);
		break;
	case VT_R4:
		output.AppendFloat(variant->fltVal);
		break;
	case VT_R8:
		output.AppendFloat((float)variant->dblVal);
		break;
	case VT_CY:
	case VT_DATE:
		Assertf(false, "Unhandled variable type %d in DXDiag info!", variant->vt);
		break;
	case VT_BSTR:
	{
		wchar_t* wstr = variant->bstrVal;

		if (!AssertVerify(wstr != NULL))
		{
			output.Append("(NULL string)");
			break;
		}
		const int max_str_len = 256;
		if (!AssertVerify(wcslen(wstr) < max_str_len))
		{
			output.Append("(string too long)");
			break;
		}

		char str[max_str_len*2+1] = {0};
		size_t size = wcstombs(str, wstr, max_str_len);
		str[size+1] = (char)0;

		if (size > 0)
		{
			if (strlen(str) > size)
				output.Append("(string size mismatch)");
			else if (size < max_str_len)
			{
				bool kosher = true;
				for (int i = 0; i < (signed)size; i++)
				{
					char c = str[i];
					if ((c < 32 || c >= 127) && c != '\n' && c != '\r')
					{
						kosher = false;
						output.Append("Bad char: ");
						output.AppendDecimal((int)c);
					}
				}
				if (kosher)
					output.Append(str);
			}
			else
				output.Append("(oversize string)");
		}
		else
			output.Append("(zero-length string)");
		break;
	}
	case VT_DISPATCH:
	case VT_ERROR:
		Assertf(false, "Unhandled variable type %d in DXDiag info!", variant->vt);
		break;
	case VT_BOOL:
		output.AppendDecimal((int)variant->boolVal);
		break;
	case VT_VARIANT:
	case VT_UNKNOWN:
	case VT_DECIMAL:
		Assertf(false, "Unhandled variable type %d in DXDiag info!", variant->vt);
		break;
	case VT_I1:
		output.AppendDecimal((int)variant->cVal);
		break;
	case VT_UI1:
		output.AppendDecimal((int)variant->bVal);
		break;
	case VT_UI2:
		output.AppendDecimal((int)variant->uiVal);
		break;
	case VT_UI4:
		output.AppendDecimal((int)variant->ulVal);
		break;
	case VT_I8:
		output.AppendDecimal((int)variant->llVal);
		break;
	case VT_UI8:
		output.AppendDecimal((int)variant->ullVal);
		break;
	case VT_INT:
		output.AppendDecimal((int)variant->intVal);
		break;
	case VT_UINT:
		output.AppendDecimal((int)variant->uintVal);
		break;
	case VT_VOID:
	case VT_HRESULT:
	case VT_PTR:
	case VT_SAFEARRAY:
	case VT_CARRAY:
	case VT_USERDEFINED:
		Assertf(false, "Unhandled variable type %d in DXDiag info!", variant->vt);
		break;
	case VT_LPSTR: // TODO: Is this ever used?
		AssertMsg(false, "VT_LPSTR type in DXDiag info!");
		break;
	case VT_LPWSTR: // TODO: Is this ever used?
		AssertMsg(false, "VT_LPWSTR type in DXDiag info!");
		break;
	case VT_RECORD:
	case VT_INT_PTR:
	case VT_UINT_PTR:
	case VT_FILETIME:
	case VT_BLOB:
	case VT_STREAM:
	case VT_STORAGE:
	case VT_STREAMED_OBJECT:
	case VT_STORED_OBJECT:
	case VT_BLOB_OBJECT:
	case VT_CF:
	case VT_CLSID:
	case VT_VERSIONED_STREAM:
	case VT_BSTR_BLOB:
	case VT_VECTOR:
	case VT_ARRAY:
	case VT_BYREF:
	case VT_RESERVED:
	case VT_ILLEGAL:
		Assertf(false, "Unhandled variable type %d in DXDiag info!", variant->vt);
		break;
		//case VT_ILLEGALMASKED:
		//case VT_TYPEMASK:
	}
}

#endif // RSG_DXDIAG_ENABLED


struct ATIDeviceName
{
	UINT m_DeviceID;
	wchar_t *m_DeviceName;
};

ATIDeviceName gATIDeviceNames[] = {
	{0x1304, L"KV SPECTRE MOBILE 35W (1304)"},
	{0x1305, L"KV SPECTRE DESKTOP 100W (1305)"},
	{0x1306, L"KV SPECTRE SL MOBILE 35W (1306)"},
	{0x1307, L"KV SPECTRE SL DESKTOP 100W (1307)"},
	{0x6600, L"AMD Radeon HD 8600/8700M"},
	{0x6601, L"AMD Radeon (TM) HD 8500M/8700M"},
	{0x6602, L"MARS (6602)"},
	{0x6603, L"MARS (6603)"},
	{0x6604, L"AMD Radeon R7 M265"},
	{0x6605, L"AMD Radeon R7 M260"},
	{0x6606, L"AMD Radeon HD 8790M"},
	{0x6607, L"AMD Radeon (TM) HD8530M"},
	{0x6610, L"AMD Radeon R7 250"},
	{0x6611, L"AMD Radeon R7 240"},
	{0x6613, L"AMD Radeon HD 8500/8600 Series"},
	{0x6620, L"MARS (6620)"},
	{0x6621, L"MARS (6621)"},
	{0x6623, L"MARS (6623)"},
	{0x6631, L"AMD Radeon R5 240"},
	{0x6640, L"SATURN (6640)"},
	{0x6641, L"SATURN (6641)"},
	{0x6649, L"BONAIRE (6649)"},
	{0x6650, L"BONAIRE (6650)"},
	{0x6651, L"AMD Radeon R7 260"},
	{0x6658, L"AMD Radeon R7 260X"},
	{0x6660, L"AMD Radeon HD 8600M Series"},
	{0x6663, L"AMD Radeon HD 8500M Series"},
	{0x6664, L"AMD Radeon R5 M240"},
	{0x6665, L"AMD Radeon R7 M230"},
	{0x6667, L"AMD Radeon R7 M230"},
	{0x6701, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6702, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6703, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6704, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6705, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6706, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6707, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6708, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6709, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6718, L"AMD Radeon HD 6900 Series"},
	{0x6719, L"AMD Radeon HD 6900 Series"},
	{0x6721, L"Mobility Radeon HD 6000 series"},
	{0x6724, L"Mobility Radeon HD 6000 series"},
	{0x6725, L"Mobility Radeon HD 6000 series"},
	{0x6728, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6729, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6738, L"AMD Radeon HD 6800 Series"},
	{0x6739, L"AMD Radeon HD 6800 Series"},
	{0x6740, L"ATI Mobility Radeon HD 6000 series"},
	{0x6741, L"AMD Radeon HD 6600M Series"},
	{0x6742, L"AMD Radeon HD 6625M Graphics"},
	{0x6743, L"AMD Radeon E6760"},
	{0x6744, L"ATI Mobility Radeon HD 6000 series"},
	{0x6749, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6750, L"AMD Radeon HD 6500 series"},
	{0x6751, L"AMD Radeon HD 7600A Series"},
	{0x6758, L"AMD Radeon HD 6670"},
	{0x6759, L"AMD Radeon HD 6570"},
	{0x6760, L"AMD Radeon HD 7400M Series"},
	{0x6761, L"AMD Radeon HD 6430M"},
	{0x6763, L"AMD Radeon E6460"},
	{0x6764, L"Mobility Radeon HD 6000 series"},
	{0x6765, L"Mobility Radeon HD 6000 series"},
	{0x6768, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6770, L"AMD Radeon HD 6400 Series"},
	{0x6771, L"AMD Radeon(TM) HD8490"},
	{0x6772, L"AMD Radeon HD 7400A Series"},
	{0x6778, L"AMD Radeon HD 7000 series"},
	{0x6779, L"AMD Radeon R5 230"},
	{0x6780, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6784, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6788, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x6798, L"AMD Radeon R9 280X"},
	{0x6799, L"AMD Radeon HD 7900 Series"},
	{0x6800, L"AMD Radeon HD 7970M"},
	{0x6801, L"AMD Radeon R9 M290X"},
	{0x6806, L"NEPTUNE (6806)"},
	{0x6808, L"ATI FirePro V(FireGL V) Graphics Adapter"},
	{0x6809, L"ATI FirePro V(FireGL V) Graphics Adapter"},
	{0x6810, L"AMD Radeon R9 270X"},
	{0x6811, L"AMD Radeon R9 270"},
	{0x6818, L"AMD Radeon HD 7800 Series"},
	{0x6819, L"AMD Radeon R7 265"},
	{0x6820, L"AMD Radeon R9 M275X"},
	{0x6821, L"AMD Radeon R9 M270X"},
	{0x6822, L"AMD Radeon R9 M265X"},
	{0x6823, L"AMD Radeon R9 M265X"},
	{0x6825, L"AMD Radeon HD 7800M Series"},
	{0x6826, L"AMD Radeon HD 7700M Series"},
	{0x6827, L"AMD Radeon HD 7800M Series"},
	{0x6828, L"ATI FirePro V(FireGL V) Graphics Adapter"},
	{0x6830, L"GREAT WALL"},
	{0x6831, L"AMD Radeon HD 7700M Series"},
	{0x6835, L"AMD Radeon R7 250"},
	{0x6837, L"AMD Radeon HD7700 Series"},
	{0x6840, L"AMD Radeon HD 7600M Series"},
	{0x6841, L"AMD Radeon HD 7500M/7600M Series"},
	{0x6842, L"AMD Radeon HD 7000M Series"},
	{0x6843, L"AMD Radeon HD 7670M"},
	{0x6849, L"AMD Radeon HD 7400 Series"},
	{0x6850, L"AMD Radeon HD 7400 Series"},
	{0x6858, L"AMD Radeon HD 7400 Series"},
	{0x6859, L"AMD Radeon HD 7400 Series"},
	{0x6880, L"EG LEXINGTON"},
	{0x6888, L"ATI FirePro V8800 (FireGL) Graphics Adapter"},
	{0x6889, L"ATI FirePro V7800 (FireGL) Graphics Adapter"},
	{0x6898, L"ATI Radeon HD 5800 Series"},
	{0x6899, L"ATI Radeon HD 5800 Series"},
	{0x6900, L"AMD Radeon R7 M260"},
	{0x6901, L"AMD Radeon R5 M255"},
	{0x6907, L"AMD Radeon R5 M255"},
	{0x6939, L"AMD Radeon R9 285"},
	{0x9440, L"AMD Radeon HD 4870"},
	{0x9441, L"AMD Radeon HD 4870 X2"},
	{0x9442, L"AMD Radeon HD 4850"},
	{0x9443, L"AMD Radeon HD 4850 X2"},
	{0x9460, L"AMD Radeon HD 4890"},
	{0x9640, L"AMD Radeon HD 6550D"},
	{0x9641, L"AMD Radeon(TM) HD 6620G"},
	{0x9642, L"AMD Radeon HD 6370D"},
	{0x9643, L"AMD Radeon(TM) HD 6380G"},
	{0x9644, L"AMD Radeon HD 6410D"},
	{0x9645, L"AMD Radeon HD 6410D"},
	{0x9647, L"AMD Radeon(TM) HD 6520G"},
	{0x9648, L"AMD Radeon(TM) HD 6480G"},
	{0x9649, L"AMD Radeon(TM) HD 6480G"},
	{0x9802, L"AMD Radeon HD 6310 Graphics"},
	{0x9803, L"AMD Radeon HD 6310 Graphics"},
	{0x9804, L"AMD Radeon HD 6250 Graphics"},
	{0x9805, L"AMD Radeon HD 6250 Graphics"},
	{0x9806, L"AMD Radeon HD 6320 Graphics"},
	{0x9807, L"AMD Radeon HD 6290 Graphics"},
	{0x9808, L"AMD Radeon HD 7340 Graphics"},
	{0x9809, L"AMD Radeon HD 7310 Graphics"},
	{0x9831, L"AMD Radeon HD 8400E"},
	{0x9832, L"AMD Radeon HD 8330"},
	{0x9833, L"AMD Radeon HD 8330E"},
	{0x9834, L"AMD Radeon HD 8210"},
	{0x9835, L"AMD Radeon HD 8210E"},
	{0x9836, L"AMD Radeon HD 8280"},
	{0x9837, L"AMD Radeon HD 8280E"},
	{0x9838, L"AMD Radeon HD 8240"},
	{0x9839, L"AMD Radeon HD 8180"},
	{0x9900, L"AMD Radeon HD 7660G"},
	{0x9901, L"AMD Radeon HD 7660D"},
	{0x9903, L"AMD Radeon HD 7640G"},
	{0x9904, L"AMD Radeon HD 7560D"},
	{0x9905, L"ATI FirePro A300 Series(FireGL V) Graphics Adapter"},
	{0x9906, L"AMD FirePro A300 Series (FireGL V) Graphics Adapter"},
	{0x9907, L"AMD Radeon HD 7620G"},
	{0x9908, L"AMD Radeon HD 7600G"},
	{0x9909, L"AMD Radeon HD 7500G"},
	{0x9910, L"AMD Radeon HD 7660G"},
	{0x9913, L"AMD Radeon HD 7640G"},
	{0x9917, L"AMD Radeon HD 7620G"},
	{0x9918, L"AMD Radeon HD 7600G"},
	{0x9919, L"AMD Radeon HD 7500G"},
	{0x9990, L"AMD Radeon HD 7520G"},
	{0x9991, L"AMD Radeon HD 7540D"},
	{0x9992, L"AMD Radeon HD 7420G"},
	{0x9993, L"AMD Radeon HD 7480D"},
	{0x9994, L"AMD Radeon HD 7400G"},
	{0x9995, L"AMD Radeon HD 8450G"},
	{0x9996, L"AMD Radeon HD 8470D"},
	{0x9997, L"AMD Radeon HD 8350G"},
	{0x9998, L"AMD Radeon HD 8370D"},
	{0x9999, L"AMD Radeon HD 8510G"},
	{0x6610, L"AMD Radeon HD 8500/8600 Series"},
	{0x665C, L"AMD Radeon HD 7700 Series"},
	{0x666F, L"SUN (666F)"},
	{0x6700, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x671C, L"AMD Radeon HD 6900 Series"},
	{0x671D, L"AMD Radeon HD 6900 Series"},
	{0x671F, L"AMD Radeon HD 6900 Series"},
	{0x6720, L"AMD Radeon HD 6900M Series"},
	{0x673E, L"AMD Radeon HD 6700 Series"},
	{0x6742, L"AMD Radeon HD 8500/8600 Series"},
	{0x674A, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x675B, L"AMD Radeon HD 8500/8600 Series"},
	{0x675D, L"AMD Radeon HD 8500/8600 Series"},
	{0x675F, L"AMD Radeon HD 5500 Series"},
	{0x677B, L"AMD Radeon R5 230"},
	{0x678A, L"ATI FirePro V (FireGL V) Graphics Adapter"},
	{0x679A, L"AMD Radeon R9 280"},
	{0x679B, L"AMD Radeon HD 7900 Series"},
	{0x679E, L"AMD Radeon HD 7800 Series"},
	{0x67B0, L"AMD Radeon R9 290X"},
	{0x67B1, L"AMD Radeon R9 290"},
	{0x67B9, L"AMD Radeon R9 295X2"},
	{0x682B, L"AMD Radeon HD 8800M Series"},
	{0x682D, L"AMD Radeon HD 7700M Series"},
	{0x682F, L"AMD Radeon HD 7700M Series"},
	{0x683D, L"AMD Radeon HD 7700 Series"},
	{0x683F, L"AMD Radeon HD 7700 Series"},
	{0x684C, L"ATI FirePro V(FireGL V) Graphics Adapter"},
	{0x688A, L"ATI FirePro V9800 (FireGL) Graphics Adapter"},
	{0x688C, L"AMD FireStream 9370"},
	{0x688D, L"AMD FireStream 9350"},
	{0x689B, L"AMD Radeon HD 6800 Series"},
	{0x689C, L"ATI Radeon HD 5900 Series"},
	{0x689D, L"ATI Radeon HD 5900 Series"},
	{0x689E, L"ATI Radeon HD 5800 Series"},
	{0x68A0, L"ATI Mobility Radeon HD 5800 Series"},
	{0x68A1, L"ATI Mobility Radeon HD 5800 Series"},
	{0x68A8, L"AMD Radeon HD 6800M Series"},
	{0x68A9, L"ATI FirePro V5800 (FireGL) Graphics Adapter"},
	{0x68B0, L"ATI Mobility Radeon HD 5800 Series"},
	{0x68B8, L"ATI Radeon HD 5700 Series"},
	{0x68B9, L"ATI Radeon HD 5600/5700"},
	{0x68BA, L"AMD Radeon HD 6700 Series"},
	{0x68BE, L"ATI Radeon HD 5700 Series"},
	{0x68BF, L"AMD Radeon HD 6700 Series"},
	{0x68C0, L"ATI Mobility Radeon HD 5000 Series"},
	{0x68C1, L"ATI Mobility Radeon HD 5000 Series"},
	{0x68C7, L"ATI Mobility Radeon HD 5570"},
	{0x68C8, L"ATI FirePro V4800 (FireGL) Graphics Adapter"},
	{0x68C9, L"ATI FirePro 3800 (FireGL) Graphics Adapter"},
	{0x68D8, L"ATI Radeon HD 5670"},
	{0x68D9, L"ATI Radeon HD 5570"},
	{0x68DA, L"ATI Radeon HD 5500 Series"},
	{0x68DE, L"EG REDWOOD"},
	{0x68E0, L"ATI Mobility Radeon HD 5000 Series"},
	{0x68E1, L"ATI Mobility Radeon HD 5000 Series"},
	{0x68E4, L"ATI Mobility Radeon Graphics"},
	{0x68E5, L"ATI Mobility Radeon Graphics"},
	{0x68E8, L"EG CEDAR"},
	{0x68E9, L"ATI FirePro (FireGL) Graphics Adapter"},
	{0x68F1, L"ATI FirePro (FireGL) Graphics Adapter"},
	{0x68F2, L"AMD FirePro 2270 (ATI FireGL)"},
	{0x68F8, L"EG CEDAR"},
	{0x68F9, L"ATI Radeon HD 5450"},
	{0x68FA, L"AMD Radeon HD 7300 Series"},
	{0x68FE, L"EG CEDAR"},
	{0x964A, L"AMD Radeon HD 6530D"},
	{0x964E, L"SUMO 964E"},
	{0x964F, L"SUMO 964F"},
	{0x980A, L"AMD Radeon HD 7290 Graphics"},
	{0x9830, L"AMD Radeon HD 8400"},
	{0x983A, L"TM EMB 2C (983A)"},
	{0x983B, L"TM 4C (983B)"},
	{0x983C, L"TM EMB 4C (983C)"},
	{0x983D, L"AMD Radeon HD 8250"},
	{0x983E, L"KB DT 4C (N-1) (983E)"},
	{0x983F, L"KB DT 2C (983F)"},
	{0x990A, L"AMD Radeon HD 7500G"},
	{0x990B, L"AMD Radeon HD 8650G"},
	{0x990C, L"AMD Radeon HD 8670D"},
	{0x990D, L"AMD Radeon HD 8550G"},
	{0x990E, L"AMD Radeon HD 8570D"},
	{0x990F, L"AMD Radeon HD 8610G"},
	{0x999A, L"AMD Radeon HD 8410G"},
	{0x999B, L"AMD Radeon HD 8310G"},
	{0x99A0, L"AMD Radeon HD 7520G"},
	{0x99A2, L"AMD Radeon HD 7420G"},
	{0x99A4, L"AMD Radeon HD 7400G"}
};

static wchar_t CardDescriptionString[256];

wchar_t* rage::DXDiag::GetCardName(DXGI_ADAPTER_DESC &oAdapterDesc)
{
	if (oAdapterDesc.VendorId == 0x1002 || oAdapterDesc.VendorId == 0x1022)
	{
		int numElements = sizeof(gATIDeviceNames) / sizeof(ATIDeviceName);
		char deviceDescPlusOne[128] = "";
		char deviceDesc[128] = "";
		char adapterDesc[128];
		WideToUtf8(adapterDesc, (rage::char16*) oAdapterDesc.Description, 128);

		for (long index = 0; index < numElements; index++)
		{
			if (gATIDeviceNames[index].m_DeviceID == oAdapterDesc.DeviceId + 1)
			{
				WideToUtf8(deviceDescPlusOne, (rage::char16*) gATIDeviceNames[index].m_DeviceName, 128);
				break;
			}
		}
		for (long index = 0; index < numElements; index++)
		{
			if (gATIDeviceNames[index].m_DeviceID == oAdapterDesc.DeviceId)
			{
				WideToUtf8(deviceDesc, (rage::char16*) gATIDeviceNames[index].m_DeviceName, 128);
				break;
			}
		}

		char description[256];
		if (strlen(deviceDescPlusOne) > 1 && strlen(deviceDesc) > 1)
		{
			formatf(description, 256, "%s (%s || %s)", adapterDesc, deviceDesc, deviceDescPlusOne);
			Utf8ToWide((rage::char16*)CardDescriptionString, description, 256);
			return CardDescriptionString;
		}
		else if (strlen(deviceDescPlusOne) > 1)
		{
			formatf(description, 256, "%s (%s)", adapterDesc, deviceDescPlusOne);
			Utf8ToWide((rage::char16*)CardDescriptionString, description, 256);
			return CardDescriptionString;
		}
		else if (strlen(deviceDesc) > 1)
		{
			formatf(description, 256, "%s (%s)", adapterDesc, deviceDesc);
			Utf8ToWide((rage::char16*)CardDescriptionString, description, 256);
			return CardDescriptionString;
		}
	}
	return oAdapterDesc.Description;
}

void rage::DXDiag::GetOsName(char* pszOS, int bufferCount)
{
	Assert(pszOS);

	// Prepare destination
	DXDiag::OutputMethod output;
	// Try to init COM
	bool cleanupCOM = SUCCEEDED(CoInitialize(NULL));
	// Try to get a provider
	IDxDiagProvider* provider = NULL;
	HRESULT hr = CoCreateInstance(CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER, IID_IDxDiagProvider, (LPVOID*) &provider);

	// Check we got it
	if (AssertVerify(SUCCEEDED(hr)) && AssertVerify(provider != NULL))
	{
		// Prepare to initialise
		DXDIAG_INIT_PARAMS dxDiagInitParams = 
		{
			sizeof(DXDIAG_INIT_PARAMS),
			DXDIAG_DX9_SDK_VERSION,
			false,
			NULL
		};

		// Try to initialise it
		if (AssertVerify(SUCCEEDED(provider->Initialize(&dxDiagInitParams))))
		{
			IDxDiagContainer* root = NULL;
			if (AssertVerify(SUCCEEDED(provider->GetRootContainer(&root))))
			{
				IDxDiagContainer* child = NULL;

				// Grab the info we need
				if (AssertVerify(SUCCEEDED(root->GetChildContainer(L"DxDiag_SystemInfo", &child))))
				{
					DXDiag::WritePropertyValue(output, child, L"szOSExEnglish");
				}
			}
		}
	}

	safecat(pszOS, output.stringBuilder.ToString(), bufferCount);

	// Cleanup COM if necessary
	if (cleanupCOM)
		CoUninitialize();
}

#endif // __WIN32PC
