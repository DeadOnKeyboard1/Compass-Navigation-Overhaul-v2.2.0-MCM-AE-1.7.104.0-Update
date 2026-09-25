#include "Settings.h"

#include <filesystem>
#include "utils/Logger.h"

namespace settings
{

	static std::string s_iniFileName = "CompassNavigationOverhaul.ini";


	static std::filesystem::path ResolvePath(const std::string& relPath)
	{
		std::error_code ec;
		std::filesystem::path p1(relPath);
		if (std::filesystem::exists(p1, ec)) return p1;

		std::filesystem::path p2 = std::filesystem::current_path(ec) / relPath;
		if (std::filesystem::exists(p2, ec)) return p2;

		return p1;
	}

	static void ParseIniDirect(const std::filesystem::path& a_path)
	{
		std::error_code ec;
		if (!std::filesystem::exists(a_path, ec)) {
			return;
		}

		std::ifstream file(a_path, std::ios::binary);
		if (!file.is_open()) {
			return;
		}

		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		std::string_view sv = content;
		if (sv.size() >= 3 &&
			static_cast<unsigned char>(sv[0]) == 0xEF &&
			static_cast<unsigned char>(sv[1]) == 0xBB &&
			static_cast<unsigned char>(sv[2]) == 0xBF) {
			sv.remove_prefix(3);
		}

		std::string currentSection;
		std::istringstream stream((std::string(sv)));
		std::string line;

		while (std::getline(stream, line)) {
			while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
			size_t start = 0;
			while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) start++;
			if (start >= line.size()) continue;
			line = line.substr(start);

			if (line.empty() || line[0] == ';' || line[0] == '#') continue;

			if (line.front() == '[' && line.back() == ']') {
				currentSection = line.substr(1, line.size() - 2);
				for (auto& c : currentSection) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				continue;
			}

			size_t eqPos = line.find('=');
			if (eqPos == std::string::npos) continue;

			std::string key = line.substr(0, eqPos);
			std::string val = line.substr(eqPos + 1);

			while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
			size_t valStart = 0;
			while (valStart < val.size() && (val[valStart] == ' ' || val[valStart] == '\t')) valStart++;
			val = val.substr(valStart);

			std::string lowerKey = key;
			for (auto& c : lowerKey) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

			auto parseBool = [](const std::string& v) -> std::optional<bool> {
				std::string lower = v;
				for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				if (lower == "1" || lower == "true") return true;
				if (lower == "0" || lower == "false") return false;
				return std::nullopt;
			};
			auto parseFloat = [](const std::string& v) -> std::optional<float> {
				try {
					size_t parsed = 0;
					const float value = std::stof(v, &parsed);
					if (parsed != v.size() || !std::isfinite(value)) return std::nullopt;
					return value;
				} catch (...) {
					return std::nullopt;
				}
			};
			auto parseUInt = [](const std::string& v) -> std::optional<std::uint32_t> {
				if (v.empty() || v.front() == '-') return std::nullopt;
				try {
					size_t parsed = 0;
					const unsigned long value = std::stoul(v, &parsed, 10);
					if (parsed != v.size() || value > (std::numeric_limits<std::uint32_t>::max)()) return std::nullopt;
					return static_cast<std::uint32_t>(value);
				} catch (...) {
					return std::nullopt;
				}
			};

			auto warnInvalid = [&]() {
				logger::warn("Ignoring invalid INI value [{}] {}='{}' from {}",
					currentSection, key, val, a_path.string());
			};
			auto assignFloat = [&](float& target) {
				if (auto parsed = parseFloat(val)) target = *parsed; else warnInvalid();
			};
			auto assignBool = [&](bool& target) {
				if (auto parsed = parseBool(val)) target = *parsed; else warnInvalid();
			};
			auto assignUInt = [&](std::uint32_t& target) {
				if (auto parsed = parseUInt(val)) target = *parsed; else warnInvalid();
			};

			if (currentSection == "questlist") {
				if (lowerKey == "fpositionx") assignFloat(questlist::positionX);
				else if (lowerKey == "fpositiony") assignFloat(questlist::positionY);
				else if (lowerKey == "fscale") assignFloat(questlist::scale);
				else if (lowerKey == "fmaxheight") assignFloat(questlist::maxHeight);
				else if (lowerKey == "bshowinexteriors") assignBool(questlist::showInExteriors);
				else if (lowerKey == "bshowininteriors") assignBool(questlist::showInInteriors);
				else if (lowerKey == "bhideincombat") assignBool(questlist::hideInCombat);
				else if (lowerKey == "fwalkingdelaytoshow") assignFloat(questlist::walkingDelayToShow);
				else if (lowerKey == "fjoggingdelaytoshow") assignFloat(questlist::joggingDelayToShow);
				else if (lowerKey == "fsprintingdelaytoshow") assignFloat(questlist::sprintingDelayToShow);
			} else if (currentSection == "compass") {
				if (lowerKey == "foffsetx") assignFloat(compass::offsetX);
				else if (lowerKey == "foffsety") assignFloat(compass::offsetY);
				else if (lowerKey == "fscale") assignFloat(compass::scale);
			} else if (currentSection == "display") {
				if (lowerKey == "busemetricunits") assignBool(display::useMetricUnits);
				else if (lowerKey == "bshowundiscoveredlocationmarkers") assignBool(display::showUndiscoveredLocationMarkers);
				else if (lowerKey == "bundiscoveredmeansunknownmarkers") assignBool(display::undiscoveredMeansUnknownMarkers);
				else if (lowerKey == "bundiscoveredmeansunknowninfo") assignBool(display::undiscoveredMeansUnknownInfo);
				else if (lowerKey == "bshowenemymarkers") assignBool(display::showEnemyMarkers);
				else if (lowerKey == "bshowenemynameundermarker") assignBool(display::showEnemyNameUnderMarker);
				else if (lowerKey == "bshowobjectiveastarget") assignBool(display::showObjectiveAsTarget);
				else if (lowerKey == "bshowotherobjectivescount") assignBool(display::showOtherObjectivesCount);
				else if (lowerKey == "bshowinteriormarkers") assignBool(display::showInteriorMarkers);
				else if (lowerKey == "fangletoshowmarkerdetails") assignFloat(display::angleToShowMarkerDetails);
				else if (lowerKey == "fangletokeepmarkerdetailsshown") assignFloat(display::angleToKeepMarkerDetailsShown);
				else if (lowerKey == "ffocusingdelaytoshow") assignFloat(display::focusingDelayToShow);
			} else if (currentSection == "debug") {
				if (lowerKey == "uloglevel") {
					std::uint32_t rawLevel = static_cast<std::uint32_t>(debug::logLevel);
					assignUInt(rawLevel);
					debug::logLevel = static_cast<logger::level>(rawLevel);
				}
			}

		}

		logger::info("Read INI settings from {}", a_path.string());
	}

	static void MigrateLegacySettingsIfNeeded(const std::filesystem::path& a_legacyIni,
		const std::filesystem::path& a_userSettingsIni)
	{
		std::error_code ec;
		if (std::filesystem::exists(a_userSettingsIni, ec)) {
			return;
		}

		ec.clear();
		if (!std::filesystem::exists(a_legacyIni, ec)) {
			return;
		}

		ec.clear();
		auto parent = a_userSettingsIni.parent_path();
		if (!parent.empty()) {
			std::filesystem::create_directories(parent, ec);
			if (ec) {
				logger::warn("Could not create MCM settings directory for legacy migration: {}", ec.message());
				return;
			}
		}

		ec.clear();
		if (std::filesystem::copy_file(a_legacyIni, a_userSettingsIni,
			std::filesystem::copy_options::none, ec)) {
			logger::info("Migrated legacy CNO settings from {} to {}",
				a_legacyIni.string(), a_userSettingsIni.string());
		} else if (ec) {
			logger::warn("Could not migrate legacy CNO settings to MCM user settings: {}", ec.message());
		}
	}

	void Reload()
	{
		std::filesystem::path pluginIniPath = ResolvePath("Data/SKSE/Plugins/" + s_iniFileName);
		std::filesystem::path mcmConfigIniPath = ResolvePath("Data/MCM/Config/CompassNavigationOverhaul/settings.ini");
		std::filesystem::path mcmSettingsIniPath = ResolvePath("Data/MCM/Settings/CompassNavigationOverhaul.ini");

		// Upgrade compatibility: original/older CNO releases stored user preferences in
		// Data/SKSE/Plugins/CompassNavigationOverhaul.ini. MCM Helper stores per-user
		// overrides in Data/MCM/Settings. Seed that file once when it does not exist so
		// a Vortex/Nexus update keeps the previous layout instead of falling back to 100%.
		MigrateLegacySettingsIfNeeded(pluginIniPath, mcmSettingsIniPath);

		// Parse in increasing priority:
		//   1) shipped MCM defaults
		//   2) legacy/original CNO INI (upgrade compatibility)
		//   3) MCM Helper per-user settings (authoritative)
		// This preserves old installations while keeping explicit MCM changes highest.
		ParseIniDirect(mcmConfigIniPath);
		ParseIniDirect(pluginIniPath);
		ParseIniDirect(mcmSettingsIniPath);

		// Sanitize values that are consumed directly by Scaleform/math code.
		// std::stof accepts textual NaN/Inf values; never forward those into UI transforms
		// or angle calculations. Restore the corresponding shipped default instead.
		auto finiteOr = [](float value, float fallback) { return std::isfinite(value) ? value : fallback; };
		questlist::positionX = finiteOr(questlist::positionX, 0.008F);
		questlist::positionY = finiteOr(questlist::positionY, 0.125F);
		questlist::scale = finiteOr(questlist::scale, 100.0F);
		questlist::maxHeight = finiteOr(questlist::maxHeight, 0.5F);
		questlist::walkingDelayToShow = finiteOr(questlist::walkingDelayToShow, 0.0F);
		questlist::joggingDelayToShow = finiteOr(questlist::joggingDelayToShow, 1.0F);
		questlist::sprintingDelayToShow = finiteOr(questlist::sprintingDelayToShow, 1.5F);
		compass::offsetX = finiteOr(compass::offsetX, 0.0F);
		compass::offsetY = finiteOr(compass::offsetY, 0.0F);
		compass::scale = finiteOr(compass::scale, 100.0F);
		display::angleToShowMarkerDetails = finiteOr(display::angleToShowMarkerDetails, 10.0F);
		display::angleToKeepMarkerDetailsShown = finiteOr(display::angleToKeepMarkerDetailsShown, 35.0F);
		display::focusingDelayToShow = finiteOr(display::focusingDelayToShow, 0.1F);

		display::angleToShowMarkerDetails = std::clamp(display::angleToShowMarkerDetails, 0.0F, 180.0F);
		display::angleToKeepMarkerDetailsShown = std::clamp(display::angleToKeepMarkerDetailsShown, 0.0F, 180.0F);
		display::focusingDelayToShow = std::max(display::focusingDelayToShow, 0.0F);
		questlist::scale = std::max(questlist::scale, 1.0F);
		questlist::maxHeight = std::max(questlist::maxHeight, 0.01F);
		questlist::walkingDelayToShow = std::max(questlist::walkingDelayToShow, 0.0F);
		questlist::joggingDelayToShow = std::max(questlist::joggingDelayToShow, 0.0F);
		questlist::sprintingDelayToShow = std::max(questlist::sprintingDelayToShow, 0.0F);
		compass::scale = std::max(compass::scale, 1.0F);

		const auto rawLogLevel = static_cast<int>(debug::logLevel);
		if (rawLogLevel < static_cast<int>(logger::level::trace) || rawLogLevel > static_cast<int>(logger::level::off)) {
			debug::logLevel = logger::level::info;
		}
		logger::set_level(debug::logLevel, debug::logLevel);

		logger::info("Settings active: QuestList(X={:.3f}, Y={:.3f}, Scale={:.1f}%, MaxH={:.2f}), Compass(OffX={:.1f}, OffY={:.1f}, Scale={:.1f}%)",
			questlist::positionX, questlist::positionY, questlist::scale, questlist::maxHeight,
			compass::offsetX, compass::offsetY, compass::scale);
	}

	void Init(const std::string& a_iniFileName)
	{
		s_iniFileName = a_iniFileName;
		Reload();
	}
}