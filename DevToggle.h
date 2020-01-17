#pragma once

#include "resource.h"

namespace fs = std::filesystem;

HINSTANCE g_hInst;
fs::path g_exePath;
HWND g_hWnd;
wil::unique_hdevinfo g_hDevInfo;

constexpr UINT WM_NOTIFYICON = WM_APP + 1;
constexpr UINT DEVICE_INDEX_MASK = 1 << 15;

#include "I18n.hpp"
#include "Util.hpp"
#include "SettingsUtil.hpp"
#include "DevUtil.hpp"
