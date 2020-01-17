#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devpkey.h>

// C RunTime Header Files
#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
