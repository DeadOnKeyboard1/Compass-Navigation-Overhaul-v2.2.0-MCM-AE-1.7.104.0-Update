#pragma once

#include "PCH.h"
#include <ShlObj.h>
#undef DispatchMessage

namespace SKSE::log
{
	using level = spdlog::level::level_enum;

	inline void set_level(level a_log_level, level a_flush_level)
	{
		spdlog::default_logger()->set_level(a_log_level);
		spdlog::default_logger()->flush_on(a_flush_level);
	}

	[[nodiscard]] inline std::optional<std::filesystem::path> get_safe_log_directory()
	{
		wchar_t* buffer{ nullptr };
		const auto result = ::SHGetKnownFolderPath(FOLDERID_Documents, KNOWN_FOLDER_FLAG::KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
		std::unique_ptr<wchar_t[], decltype(&::CoTaskMemFree)> knownPath(buffer, ::CoTaskMemFree);
		if (!knownPath || result != S_OK) {
			return std::nullopt;
		}

		std::filesystem::path path = knownPath.get();
		path /= "My Games";
		path /= std::filesystem::exists("steam_api64.dll") ? "Skyrim Special Edition" : "Skyrim Special Edition GOG";
		path /= "SKSE"sv;
		return path;
	}

	inline bool init(const std::string_view& a_log_name)
	{
		auto logDir = get_safe_log_directory();
		if (!logDir)
		{
			return false;
		}

		std::filesystem::path path = *logDir / a_log_name;
		path += ".log";
		std::shared_ptr<spdlog::logger> log = spdlog::basic_logger_mt("global log", path.string(), true);

		spdlog::set_default_logger(std::move(log));

		set_level(level::info, level::info);

		spdlog::set_pattern("%D - %H:%M:%S.%f [%^%l%$] %v");

		return true;
	}

	inline void flush()
	{
		spdlog::default_logger()->flush();
	}

	template <class... Args>
	void at_level(spdlog::level::level_enum a_level, fmt::format_string<Args...> a_fmt, Args&&... args)
	{
		switch (a_level) {
		case level::trace:
			trace<Args...>(a_fmt, std::forward<Args&&>(args)...);
			break;
		case level::debug:
			debug<Args...>(a_fmt, std::forward<Args&&>(args)...);
			break;
		case level::info:
			info<Args...>(a_fmt, std::forward<Args&&>(args)...);
			break;
		case level::warn:
			warn<Args...>(a_fmt, std::forward<Args&&>(args)...);
			break;
		case level::err:
			error<Args...>(a_fmt, std::forward<Args&&>(args)...);
			break;
		case level::critical:
			critical<Args...>(a_fmt, std::forward<Args&&>(args)...);
			break;
		}
	}
}

namespace GFxLogger
{
	struct info;
	struct error;

	bool RegisterStaticFunctions(RE::GFxMovieView* a_view);
}

namespace logger = SKSE::log;