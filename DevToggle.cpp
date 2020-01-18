#include "pch.h"
#include "DevToggle.h"

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	g_hInst = hInstance;
	g_exePath = GetModuleFsPath(hInstance);
	SetCurrentDirectoryW(g_exePath.c_str());
	LoadTranslateData();

#ifndef _WIN64
	BOOL isWow64;
	if (!IsWow64Process(GetCurrentProcess(), &isWow64))
		isWow64 = FALSE;

	if (isWow64)
	{
		int select;
		if (TaskDialog(nullptr, hInstance, _(L"DevToggle"), nullptr, _(L"32bit applications can not enable/disable devices on 64bit OS, do you want to continue?"), TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON, &select) != S_OK || select != IDYES)
			return EXIT_FAILURE;
	}
#endif

	LoadSettings();
	LoadDevices();

	WNDCLASSEXW wcex = {
		.cbSize = sizeof(wcex),
		.lpfnWndProc = WndProc,
		.hInstance = hInstance,
		.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_DEVTOGGLE)),
		.hCursor = LoadCursorW(nullptr, IDC_ARROW),
		.lpszClassName = L"DevToggle",
		.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_DEVTOGGLE))
	};

	RegisterClassExW(&wcex);

	g_hWnd = CreateWindowW(L"DevToggle", nullptr, WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	if (!g_hWnd)
		return EXIT_FAILURE;

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return static_cast<int>(msg.wParam);
}

void ShowContextMenu(HWND hWnd, int x, int y)
{
	try
	{
		wil::unique_hmenu hMenu(CreatePopupMenu());
		THROW_LAST_ERROR_IF_NULL(hMenu);

		auto hdc = wil::GetDC(nullptr);
		THROW_IF_NULL_ALLOC(hdc);

		wil::unique_hdc hMemDC(CreateCompatibleDC(hdc.get()));
		THROW_IF_NULL_ALLOC(hMemDC);

		std::vector<wil::unique_hbitmap> hBitmaps;

		int cx = GetSystemMetrics(SM_CXSMICON), cy = GetSystemMetrics(SM_CYSMICON);

		SP_DEVINFO_DATA devInfo = { sizeof(devInfo) };
		for (DWORD i = 0; SetupDiEnumDeviceInfo(g_hDevInfo.get(), i, &devInfo); ++i)
		{
			std::wstring str(64, L'\0');
			auto cr = GetDeviceStringProperty(devInfo.DevInst, &DEVPKEY_Device_FriendlyName, str);
			if (cr != CR_SUCCESS)
			{
				cr = GetDeviceStringProperty(devInfo.DevInst, &DEVPKEY_Device_DeviceDesc, str);
				if (cr != CR_SUCCESS)
				{
					str.resize(MAX_DEVICE_ID_LEN);
					cr = CM_Get_Device_IDW(devInfo.DevInst, str.data(), MAX_DEVICE_ID_LEN, 0);
					if (cr != CR_SUCCESS)
						str = _(L"Unknown Device");
				}
			}

			auto hBitmap = DrawDeviceIconBitmap(g_hDevInfo.get(), &devInfo, cx, cy, hdc.get(), hMemDC.get());
			hBitmaps.emplace_back(hBitmap);

			MENUITEMINFOW mi = {
				.cbSize = sizeof(mi),
				.fMask = MIIM_ID | MIIM_STRING | MIIM_BITMAP,
				.wID = i | DEVICE_INDEX_MASK,
				.dwTypeData = str.data(),
				.hbmpItem = hBitmap
			};
			InsertMenuItemW(hMenu.get(), UINT_MAX, TRUE, &mi);
		}

		AppendMenuW(hMenu.get(), MF_SEPARATOR, 0, nullptr);

		/*MENUITEMINFOW mi = {
				.cbSize = sizeof(mi),
				.fMask = MIIM_STATE | MIIM_ID | MIIM_STRING,
				.fState = MFS_DEFAULT,
				.wID = IDM_SETTINGS,
				.dwTypeData = const_cast<LPWSTR>(_(L"Settings"))
		};
		InsertMenuItemW(hMenu.get(), UINT_MAX, TRUE, &mi);*/

		AppendMenuW(hMenu.get(), MF_STRING, IDM_EXIT, _(L"E&xit"));

		THROW_IF_WIN32_BOOL_FALSE(SetForegroundWindow(hWnd));

		UINT flags = TPM_RIGHTBUTTON;
		if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
			flags |= TPM_RIGHTALIGN;
		else
			flags |= TPM_LEFTALIGN;

		TrackPopupMenuEx(hMenu.get(), flags, x, y, hWnd, nullptr);
	}
	CATCH_LOG_RETURN();
}

void ShowBalloon(HWND hWnd, const wchar_t* info, DWORD infoFlags = NIIF_INFO)
{
	NOTIFYICONDATA nid = {
		.cbSize = sizeof(nid),
		.hWnd = hWnd,
		.uFlags = NIF_INFO,
		.dwInfoFlags = infoFlags | NIIF_RESPECT_QUIET_TIME
	};
	wcscpy_s(nid.szInfoTitle, _(L"DevToggle"));
	wcscpy_s(nid.szInfo, info);
	LOG_IF_WIN32_BOOL_FALSE(Shell_NotifyIconW(NIM_MODIFY, &nid));
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT WM_TASKBAR_CREATED;
	static NOTIFYICONDATAW nid = {
		.cbSize = sizeof(nid),
		.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP,
		.uCallbackMessage = WM_NOTIFYICON,
		.uVersion = NOTIFYICON_VERSION_4
	};
	switch (message)
	{
	case WM_CREATE:
	{
		nid.hWnd = hWnd;
		nid.hIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_DEVTOGGLE));
		wcscpy_s(nid.szTip, _(L"DevToggle"));

		FAIL_FAST_IF_WIN32_BOOL_FALSE(Shell_NotifyIconW(NIM_ADD, &nid));
		FAIL_FAST_IF_WIN32_BOOL_FALSE(Shell_NotifyIconW(NIM_SETVERSION, &nid));

		WM_TASKBAR_CREATED = RegisterWindowMessageW(L"TaskbarCreated");
		LOG_LAST_ERROR_IF(WM_TASKBAR_CREATED == 0);
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		if (wmId & DEVICE_INDEX_MASK)
		{
			wmId &= ~DEVICE_INDEX_MASK;
			try
			{
				SP_DEVINFO_DATA devInfo = { sizeof(devInfo) };
				if (SetupDiEnumDeviceInfo(g_hDevInfo.get(), wmId, &devInfo))
				{
					DWORD stateChange = IsDeviceDisabled(devInfo.DevInst) ? DICS_ENABLE : DICS_DISABLE;
					EnableOrDisableDevice(g_hDevInfo.get(), &devInfo, stateChange);
					
					SP_DEVINSTALL_PARAMS_W devParams;
					devParams.cbSize = sizeof(devParams);

					// Needs reboot
					if (SetupDiGetDeviceInstallParamsW(g_hDevInfo.get(), &devInfo, &devParams) && (devParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)))
						ShowBalloon(hWnd, _(L"The device needs reboot to change its state."), NIIF_WARNING);
				}
			}
			catch (...)
			{
				LOG_CAUGHT_EXCEPTION();
				ShowBalloon(hWnd, _(L"Failed to toggle device state."), NIIF_ERROR);
			}
		}
		else
		{
			// Parse the menu selections:
			switch (wmId)
			{
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			case IDM_SETTINGS:
				break;
			default:
				return DefWindowProcW(hWnd, message, wParam, lParam);
			}
		}
	}
	break;
	case WM_DESTROY:
		g_hDevInfo.reset();
		Shell_NotifyIconW(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;
	case WM_NOTIFYICON:
		switch (LOWORD(lParam))
		{
		case NIN_SELECT:
		case NIN_KEYSELECT:
			PostMessageW(hWnd, WM_COMMAND, IDM_SETTINGS, 0);
			break;
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd, LOWORD(wParam), HIWORD(wParam));
			break;
		}
		break;
	default:
		if (WM_TASKBAR_CREATED && message == WM_TASKBAR_CREATED)
		{
			if (!Shell_NotifyIconW(NIM_MODIFY, &nid))
			{
				FAIL_FAST_IF_WIN32_BOOL_FALSE(Shell_NotifyIconW(NIM_ADD, &nid));
				FAIL_FAST_IF_WIN32_BOOL_FALSE(Shell_NotifyIconW(NIM_SETVERSION, &nid));
			}
		}
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}
