#pragma once

#define SETUPAPI_IDI_WARN_OVERLAY 500
#define SETUPAPI_IDI_DOWN_OVERLAY 501
#define SETUPAPI_IDI_INFO_OVERLAY 502

void LoadDevices()
{
	g_hDevInfo.reset(SetupDiCreateDeviceInfoList(nullptr, nullptr));
	THROW_LAST_ERROR_IF(g_hDevInfo.get() == INVALID_HANDLE_VALUE);

	for (const auto& instId : g_settings.devices)
		THROW_IF_WIN32_BOOL_FALSE(SetupDiOpenDeviceInfoW(g_hDevInfo.get(), instId.c_str(), nullptr, 0, nullptr));
}

bool IsDeviceDisabled(DEVINST devInst)
{
	ULONG status, problem;
	auto cr = CM_Get_DevNode_Status(&status, &problem, devInst, 0);
	return (cr == CR_SUCCESS && (status & DN_HAS_PROBLEM) && problem == CM_PROB_DISABLED);
}

HBITMAP DrawDeviceIconBitmap(HDEVINFO hDevInfo, PSP_DEVINFO_DATA devInfo, int cx, int cy, HDC hdc, HDC hMemDC)
{
	BITMAPINFO bmi = { .bmiHeader = {
		.biSize = sizeof(BITMAPINFOHEADER),
		.biWidth = cx,
		.biHeight = cy,
		.biPlanes = 1,
		.biBitCount = 32,
		.biCompression = BI_RGB,
		.biSizeImage = 4UL * cx * cy
	} };
	void* bits;
	wil::unique_hbitmap hBitmap(CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0));
	THROW_IF_NULL_ALLOC(hBitmap);

	auto select = wil::SelectObject(hMemDC, hBitmap.get());

	wil::unique_hicon hIcon;
	THROW_IF_WIN32_BOOL_FALSE(SetupDiLoadDeviceIcon(hDevInfo, devInfo, cx, cy, 0, &hIcon));

	THROW_IF_WIN32_BOOL_FALSE(DrawIconEx(hMemDC, 0, 0, hIcon.get(), cx, cy, 0, nullptr, DI_NORMAL));

	if (IsDeviceDisabled(devInfo->DevInst))
	{
		static HICON hIconDownOverlay = nullptr;
		if (!hIconDownOverlay)
			hIconDownOverlay = static_cast<HICON>(LoadImageW(GetModuleHandleW(L"setupapi.dll"), MAKEINTRESOURCEW(SETUPAPI_IDI_DOWN_OVERLAY), IMAGE_ICON, cx, cy, 0));
		THROW_IF_WIN32_BOOL_FALSE(DrawIconEx(hMemDC, 0, 0, hIconDownOverlay, cx, cy, 0, nullptr, DI_NORMAL));
	}

	return hBitmap.release();
}

CONFIGRET GetDeviceStringProperty(DEVINST devInst, const DEVPROPKEY* propKey, std::wstring& str)
{
	CONFIGRET cr = CR_SUCCESS;
	str.resize(64);
	while (1)
	{
		DEVPROPTYPE type;
		ULONG size = static_cast<ULONG>(str.size() * sizeof(wchar_t));
		cr = CM_Get_DevNode_PropertyW(devInst, propKey, &type, reinterpret_cast<PBYTE>(str.data()), &size, 0);
		if (cr == CR_SUCCESS || cr == CR_BUFFER_SMALL)
		{
			if (type != DEVPROP_TYPE_STRING)
			{
				str.clear();
				return CR_INVALID_DATA;
			}
			if (cr == CR_BUFFER_SMALL)
			{
				str.resize(size / sizeof(wchar_t));
				continue;
			}
			// cr == CR_SUCCESS
			str.resize((size / sizeof(wchar_t)) - 1);
			break;
		}
		else
		{
			str.clear();
			return cr;
		}
	}
	return cr;
}

void EnableOrDisableDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA devInfo, DWORD stateChange)
{
	SP_PROPCHANGE_PARAMS pcp = {
		.ClassInstallHeader = {
			.cbSize = sizeof(SP_CLASSINSTALL_HEADER),
			.InstallFunction = DIF_PROPERTYCHANGE
		},
		.StateChange = stateChange,
		.Scope = DICS_FLAG_CONFIGSPECIFIC,
		.HwProfile = 0
	};

	if (stateChange == DICS_ENABLE)
	{
		pcp.Scope = DICS_FLAG_GLOBAL;
		if (SetupDiSetClassInstallParamsW(hDevInfo, devInfo, &pcp.ClassInstallHeader, sizeof(pcp)))
			SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, devInfo);
		pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
	}
	
	THROW_IF_WIN32_BOOL_FALSE(SetupDiSetClassInstallParamsW(hDevInfo, devInfo, &pcp.ClassInstallHeader, sizeof(pcp)));
	THROW_IF_WIN32_BOOL_FALSE(SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, devInfo));
}
