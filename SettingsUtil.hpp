#pragma once

#include "RapidjsonUtil.hpp"

struct Settings
{
	std::vector<std::wstring> devices;
};

Settings g_settings;

void DefaultSettings()
{
	g_settings.devices.clear();
}

void LoadSettings()
{
	DefaultSettings();

	wil::unique_file file;
	if (fopen_s(&file, "settings.json", "rb") != 0)
		return;

	char readBuffer[1024];
	rapidjson::FileReadStream is(file.get(), readBuffer, sizeof(readBuffer));

	rapidjson::Document dom;

	dom.ParseStream(is);
	THROW_HR_IF(E_INVALIDARG, dom.HasParseError());
	THROW_HR_IF(E_INVALIDARG, !dom.IsObject());

	for (auto it = dom.MemberBegin(); it != dom.MemberEnd(); ++it)
	{
		if (it->name == "devices" && it->value.IsObject())
		{
			g_settings.devices.reserve(it->value.MemberCount());
			for (auto mit = it->value.MemberBegin(); mit != it->value.MemberEnd(); ++mit)
			{
				std::wstring ws;
				ws.reserve(mit->name.GetStringLength());

				if (!UTF8CStrToUTF16WString(mit->name.GetString(), ws))
					continue;

				g_settings.devices.emplace_back(ws);
			}
		}
	}
}
