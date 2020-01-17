// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

// wil
#ifndef _DEBUG
#define RESULT_DIAGNOSTICS_LEVEL 1
#endif

#include <wil/common.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <wil/result.h>

namespace wil
{
#if defined(_INC_SETUPAPI) && !defined(__WIL_INC_SETUPAPI) && !defined(WIL_KERNEL_MODE)
#define __WIL_INC_SETUPAPI
	typedef unique_any<HDEVINFO, decltype(&::SetupDiDestroyDeviceInfoList), ::SetupDiDestroyDeviceInfoList> unique_hdevinfo;
#endif
} // namespace wil

// rapidjson
#pragma warning(push)
#pragma warning(disable:5054)
#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/encodings.h"
#include "rapidjson/filereadstream.h"
#pragma warning(pop)

#endif //PCH_H
