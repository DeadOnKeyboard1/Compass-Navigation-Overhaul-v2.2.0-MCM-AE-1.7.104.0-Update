#include "Hooks.h"

#include "Settings.h"

#include "HUDMarkerManager.h"
#include "RE/T/TESDataHandler.h"
#include "utils/Geometry.h"

namespace hooks
{
	namespace
	{
		void LogMarkerHookOnce(std::atomic_bool& a_flag, std::string_view a_name, bool a_addResult,
			const RE::HUDMarker::ScaleformData* a_markerData, const RE::NiPoint3* a_pos)
		{
			bool expected = false;
			if (a_flag.compare_exchange_strong(expected, true)) {
				logger::info("{} marker hook reached: AddMarker={}, markerData={}, pos={}",
					a_name, a_addResult, a_markerData != nullptr, a_pos != nullptr);
			}
		}

		std::atomic_bool s_loggedLocationHook{ false };
		std::atomic_bool s_loggedEnemyHook{ false };
		std::atomic_bool s_loggedPlayerHook{ false };
	}
	static inline RE::BSTArray<RE::BGSInstancedQuestObjective>& GetPlayerObjectives(RE::PlayerCharacter* a_player)
	{
		return a_player->GetPlayerRuntimeData().objectives;
	}


	static std::optional<std::uint32_t> TryGetMarkerIndex(const RE::HUDMarkerManager* a_manager,
		const RE::HUDMarker::ScaleformData* a_markerData)
	{
		if (!a_manager) {
			return std::nullopt;
		}

		// AddMarker advances currentMarkerIndex after writing the engine-owned slot.
		// This is the authoritative index used by the original CNO implementation.
		if (auto index = a_manager->GetLastMarkerIndex()) {
			return index;
		}

		// Defensive fallback for callers that hand us a direct slot pointer.
		if (!a_markerData) {
			return std::nullopt;
		}
		const auto* begin = std::addressof(a_manager->scaleformMarkerData[0]);
		const auto* end = begin + 49;
		if (a_markerData < begin || a_markerData >= end) {
			return std::nullopt;
		}
		return static_cast<std::uint32_t>(a_markerData - begin);
	}

	namespace compat
	{
		namespace AlternatePerspective
		{
			static bool s_dataLoaded = false;
			static bool s_installed = false;
			static RE::TESQuest* s_startCellQuest = nullptr;
			static RE::TESObjectCELL* s_startCell = nullptr;

			void OnDataLoaded()
			{
				s_dataLoaded = true;
				s_installed = false;
				s_startCellQuest = nullptr;
				s_startCell = nullptr;

				if (auto* dataHandler = RE::TESDataHandler::GetSingleton()) {
					s_installed = dataHandler->LookupLoadedModByName("AlternatePerspective.esp") != nullptr;
					if (s_installed) {
						s_startCellQuest = dataHandler->LookupForm<RE::TESQuest>(0x4246F9, "AlternatePerspective.esp");
						s_startCell = dataHandler->LookupForm<RE::TESObjectCELL>(0x2C1D00, "AlternatePerspective.esp");
						SKSE::log::info("Alternate Perspective detected: start-room compass marker suppression enabled.");
					}
				}
			}

			static bool ShouldSuppressMarker(const RE::RefHandle& a_refHandle, RE::PlayerCharacter* a_player)
			{
				if (!s_dataLoaded || !s_installed || !s_startCellQuest || !s_startCell || !a_player) {
					return false;
				}

				auto* currentCell = a_player->GetParentCell();
				if (!currentCell || currentCell == s_startCell) {
					return false;
				}

				for (auto* objective : s_startCellQuest->objectives) {
					if (!objective || !objective->targets) {
						continue;
					}
					for (std::uint32_t i = 0; i < objective->numTargets; ++i) {
						auto* target = objective->targets[i];
						if (!target) {
							continue;
						}
						RE::ObjectRefHandle trackingRef;
						target->GetTrackingRef(trackingRef, s_startCellQuest);
						if (trackingRef && trackingRef.native_handle() == a_refHandle) {
							return true;
						}
					}
				}
				return false;
			}
		}
	}

	bool UpdateQuests(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
					  RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		// This hook replaces Skyrim's original AddMarker call. Never suppress the
		// vanilla marker merely because an auxiliary CNO argument is null.
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player && compat::AlternatePerspective::ShouldSuppressMarker(a_refHandle, player)) {
			return false;
		}

		if (!HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame)) {
			return false;
		}

		RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();
		auto markerIndex = TryGetMarkerIndex(a_hudMarkerManager, a_markerData);
		if (!player || !marker || !markerIndex) {
			return true;
		}

		auto& playerObjectives = GetPlayerObjectives(player);
		for (int ageIndex = static_cast<int>(playerObjectives.size()) - 1; ageIndex >= 0; --ageIndex) {
			auto* playerObjective = std::addressof(playerObjectives[ageIndex]);
			if (playerObjective->InstanceState != RE::QUEST_OBJECTIVE_STATE::kDisplayed) {
				continue;
			}

			auto* questObjective = playerObjective->Objective;
			if (!questObjective || !questObjective->targets) {
				continue;
			}

			auto* quest = questObjective->ownerQuest;
			if (!quest || !quest->IsRunning()) {
				continue;
			}

			for (std::uint32_t j = 0; j < questObjective->numTargets; ++j) {
				auto* target = questObjective->targets[j];
				if (!target) {
					continue;
				}

				RE::ObjectRefHandle trackingRef;
				target->GetTrackingRef(trackingRef, quest);
				if (trackingRef && trackingRef.native_handle() == a_refHandle) {
					CNO::HUDMarkerManager::GetSingleton()->ProcessQuestMarker(
						quest, playerObjective, ageIndex, marker, a_markerGotoFrame, *markerIndex);
					break;
				}
			}
		}

		return true;
	}

	RE::TESWorldSpace* AllowedToShowMapMarker(const RE::TESObjectREFR* a_marker)
	{
		if (!a_marker) {
			return nullptr;
		}

		RE::TESWorldSpace* markerWorldspace = a_marker->GetWorldspace();

		if (settings::display::showInteriorMarkers)
		{
			auto player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return markerWorldspace;
			}

			RE::TESWorldSpace* playerWorldspace = player->GetWorldspace();

			if (playerWorldspace && markerWorldspace && playerWorldspace != markerWorldspace)
			{
				if (!playerWorldspace->parentWorld && markerWorldspace->parentWorld == playerWorldspace) 
				{
					return playerWorldspace;
				}
			}
		}
			
		return markerWorldspace;
	}

	bool UpdateLocations(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
						   RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		// IMPORTANT: this function replaces Skyrim's AddMarker call. Preserve the
		// engine call first and make every CNO enhancement best-effort afterwards.
		// Returning early before AddMarker is what caused normal compass markers to
		// disappear while quest markers still worked.
		auto* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

		// The original CNO option intentionally hides undiscovered locations. Only
		// apply that filter when we can safely resolve valid map-marker data.
		if (marker) {
			if (auto* mapMarker = marker->extraList.GetByType<RE::ExtraMapMarker>(); mapMarker && mapMarker->mapData) {
				if (!settings::display::showUndiscoveredLocationMarkers &&
					!mapMarker->mapData->flags.all(RE::MapMarkerData::Flag::kVisible)) {
					return false;
				}
			}
		}

		const bool added = HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame);
		LogMarkerHookOnce(s_loggedLocationHook, "Location", added, a_markerData, a_pos);
		if (!added) {
			return false;
		}

		if (!marker) {
			return true;
		}

		auto* mapMarker = marker->extraList.GetByType<RE::ExtraMapMarker>();
		if (!mapMarker || !mapMarker->mapData) {
			return true;
		}

		if (auto markerIndex = TryGetMarkerIndex(a_hudMarkerManager, a_markerData)) {
			CNO::HUDMarkerManager::GetSingleton()->ProcessLocationMarker(
				mapMarker, marker, a_markerGotoFrame, *markerIndex, a_markerData);
		}
		return true;
	}

	bool UpdateEnemies(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
							RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		if (!settings::display::showEnemyMarkers) {
			return false;
		}

		// Preserve Skyrim's marker creation first. CNO metadata is optional.
		const bool added = HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame);
		LogMarkerHookOnce(s_loggedEnemyHook, "Enemy", added, a_markerData, a_pos);
		if (!added) {
			return false;
		}

		auto* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();
		if (!marker) {
			return true;
		}

		auto* enemy = marker->As<RE::Character>();
		if (!enemy) {
			return true;
		}

		if (auto markerIndex = TryGetMarkerIndex(a_hudMarkerManager, a_markerData)) {
			CNO::HUDMarkerManager::GetSingleton()->ProcessEnemyMarker(enemy, a_markerGotoFrame, *markerIndex);
		}
		return true;
	}

	bool UpdatePlayerSetMarker(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
								RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		// Preserve the vanilla player-set marker even when CNO cannot resolve its
		// optional metadata/pointer state.
		const bool added = HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame);
		LogMarkerHookOnce(s_loggedPlayerHook, "Player-set", added, a_markerData, a_pos);
		if (!added) {
			return false;
		}

		auto* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();
		if (!marker) {
			return true;
		}

		if (auto markerIndex = TryGetMarkerIndex(a_hudMarkerManager, a_markerData)) {
			CNO::HUDMarkerManager::GetSingleton()->ProcessPlayerSetMarker(marker, a_markerGotoFrame, *markerIndex);
		}
		return true;
	}

	void UpdateCompass(RE::Compass* a_compass)
	{
		if (!a_compass) {
			return;
		}

		hooks::Compass::Update(a_compass);
		CNO::HUDMarkerManager::GetSingleton()->SetMarkersExtraInfo();
	}

	namespace compat
	{
		RE::GFxMovieDef* MapMarkerFramework::GetCompassMovieDef(void*, RE::GFxMovieView* a_movieView)
		{
			// During HUD rebuilds the CNO-patched movie definition is temporarily unavailable.
			// Preserve CoMAP's original behaviour instead of returning a null/stale definition.
			if (compassMovieDef) {
				return compassMovieDef;
			}
			return a_movieView ? a_movieView->GetMovieDef() : nullptr;
		}
	}
}
