#include "Hooks.h"
#include "Settings.h"

#include "utils/Logger.h"

extern const SKSE::LoadInterface* skse;
void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg);

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	skse = a_skse;

	const SKSE::PluginDeclaration* plugin = SKSE::PluginDeclaration::GetSingleton();

	if (!logger::init(plugin->GetName()))
	{
		return false;
	}

	logger::info("Loading {} {}...", plugin->GetName(), plugin->GetVersion());

	SKSE::Init(a_skse);

	const auto runtimeVersion = REL::Module::get().version();
	if (runtimeVersion != REL::Version{ 1, 7, 104, 0 }) {
		logger::critical("Unsupported Skyrim runtime {}.{}.{}.{}; this build is restricted to 1.7.104.0 because it installs runtime-specific patch-site hooks.",
			runtimeVersion.major(), runtimeVersion.minor(), runtimeVersion.patch(), runtimeVersion.build());
		return false;
	}

	settings::Init(std::string(plugin->GetName()) + ".ini");

	logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

	if (!SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageListener))
	{
		return false;
	}

	if (!hooks::Install()) {
		logger::critical("Failed to install validated Skyrim 1.7.104.0 hooks; aborting plugin load to avoid patching unknown code");
		return false;
	}

	logger::set_level(logger::level::info, logger::level::info);
	logger::info("Marker settings: undiscovered={}, enemies={}, interior={}, objectiveTarget={}",
		settings::display::showUndiscoveredLocationMarkers, settings::display::showEnemyMarkers,
		settings::display::showInteriorMarkers, settings::display::showObjectiveAsTarget);
	logger::info("Succesfully loaded!");

	logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

	return true;
}
