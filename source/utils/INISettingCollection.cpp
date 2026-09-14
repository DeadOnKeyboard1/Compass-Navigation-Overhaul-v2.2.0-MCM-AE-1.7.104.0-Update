#include "utils/INISettingCollection.h"

#include "utils/Logger.h"

namespace utils
{
	INISettingCollection::INISettingCollection() noexcept
	{
		REL::Relocation<std::uintptr_t> __vTable(*reinterpret_cast<std::uintptr_t*>(this));

		// Use the Skyrim's INISettingCollection virtual functions
		static std::uintptr_t* skyrimsVTable = REL::Relocation<std::uintptr_t*>{ vTableId }.get();

		// Replace all except the destructor (index 0)
		for (int i = 1; i < 10; i++) {
			__vTable.write_vfunc(i, skyrimsVTable[i]);
		}
	}

	bool INISettingCollection::ReadFromPath(const std::filesystem::path& a_path)
	{
		std::string pathStr = a_path.string();
		if (!pathStr.empty()) {
			if (pathStr != subKey) {
				strcpy_s(subKey, pathStr.c_str());
			}
		} else {
			subKey[0] = '\0';
		}

		if (_this()->OpenHandle(false)) {
			_this()->ReadAllSettings();
			_this()->CloseHandle();

			return true;
		}

		return false;
	}

	bool INISettingCollection::ReadFromFile(std::string_view a_fileName)
	{
		std::filesystem::path iniPath = std::filesystem::current_path().append("Data\\SKSE\\Plugins").append(a_fileName);
		return ReadFromPath(iniPath);
	}
}