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

			auto parseBool = [](const std::string& v) -> bool {
				return v == "1" || v == "true" || v == "True" || v == "TRUE";
			};
			auto parseFloat = [](const std::string& v) -> float {
				try { return std::stof(v); } catch (...) { return 0.0F; }
			};
			auto parseUInt = [](const std::string& v) -> std::uint32_t {
				try { return static_cast<std::uint32_t>(std::stoul(v)); } catch (...) { return 0; }
			};

			if (currentSection == "questlist") {
				if (lowerKey == "fpositionx") questlist::positionX = parseFloat(val);
				else if (lowerKey == "fpositiony") questlist::positionY = parseFloat(val);
				else if (lowerKey == "fscale") questlist::scale = parseFloat(val);
				else if (lowerKey == "fmaxheight") questlist::maxHeight = parseFloat(val);
				else if (lowerKey == "bshowinexteriors") questlist::showInExteriors = parseBool(val);
				else if (lowerKey == "bshowininteriors") questlist::showInInteriors = parseBool(val);
				else if (lowerKey == "bhideincombat") questlist::hideInCombat = parseBool(val);
				else if (lowerKey == "fwalkingdelaytoshow") questlist::walkingDelayToShow = parseFloat(val);
				else if (lowerKey == "fjoggingdelaytoshow") questlist::joggingDelayToShow = parseFloat(val);
				else if (lowerKey == "fsprintingdelaytoshow") questlist::sprintingDelayToShow = parseFloat(val);
			} else if (currentSection == "compass") {
				if (lowerKey == "foffsetx") compass::offsetX = parseFloat(val);
				else if (lowerKey == "foffsety") compass::offsetY = parseFloat(val);
				else if (lowerKey == "fscale") compass::scale = parseFloat(val);
			} else if (currentSection == "display") {
				if (lowerKey == "busemetricunits") display::useMetricUnits = parseBool(val);
				else if (lowerKey == "bshowundiscoveredlocationmarkers") display::showUndiscoveredLocationMarkers = parseBool(val);
				else if (lowerKey == "bundiscoveredmeansunknownmarkers") display::undiscoveredMeansUnknownMarkers = parseBool(val);
				else if (lowerKey == "bundiscoveredmeansunknowninfo") display::undiscoveredMeansUnknownInfo = parseBool(val);
				else if (lowerKey == "bshowenemymarkers") display::showEnemyMarkers = parseBool(val);
				else if (lowerKey == "bshowenemynameundermarker") display::showEnemyNameUnderMarker = parseBool(val);
				else if (lowerKey == "bshowobjectiveastarget") display::showObjectiveAsTarget = parseBool(val);
				else if (lowerKey == "bshowotherobjectivescount") display::showOtherObjectivesCount = parseBool(val);
				else if (lowerKey == "bshowinteriormarkers") display::showInteriorMarkers = parseBool(val);
				else if (lowerKey == "fangletoshowmarkerdetails") display::angleToShowMarkerDetails = parseFloat(val);
				else if (lowerKey == "fangletokeepmarkerdetailsshown") display::angleToKeepMarkerDetailsShown = parseFloat(val);
				else if (lowerKey == "ffocusingdelaytoshow") display::focusingDelayToShow = parseFloat(val);
			} else if (currentSection == "debug") {
				if (lowerKey == "uloglevel") debug::logLevel = static_cast<logger::level>(parseUInt(val));
			}
		}

		logger::info("Read INI settings from {}", a_path.string());
	}

	void Reload()
	{
		std::filesystem::path pluginIniPath = ResolvePath("Data/SKSE/Plugins/" + s_iniFileName);
		std::filesystem::path mcmConfigIniPath = ResolvePath("Data/MCM/Config/CompassNavigationOverhaul/settings.ini");
		std::filesystem::path mcmSettingsIniPath = ResolvePath("Data/MCM/Settings/CompassNavigationOverhaul.ini");

		// Parse in increasing priority. MCM's per-user settings override the shipped
		// plugin defaults without touching Skyrim's internal INI/VTable machinery.
		ParseIniDirect(pluginIniPath);
		ParseIniDirect(mcmConfigIniPath);
		ParseIniDirect(mcmSettingsIniPath);

		// Sanitize values that are consumed directly by Scaleform/math code.
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