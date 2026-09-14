#include "Hooks.h"

#include "Settings.h"

#include "HUDMarkerManager.h"
#include "RE/B/BGSStoryTeller.h"
#include "RE/T/TESDataHandler.h"

namespace hooks
{
	static inline RE::BSTArray<RE::BGSInstancedQuestObjective>& GetPlayerObjectives(RE::PlayerCharacter* a_player)
	{
		const auto version = REL::Module::get().version();
		std::uintptr_t offset = 0x580;
		if (version >= REL::Version{ 1, 6, 1130, 0 }) {
			offset = 0x590;
		} else if (version >= REL::Version{ 1, 6, 629, 0 }) {
			offset = 0x588;
		}
		return *reinterpret_cast<RE::BSTArray<RE::BGSInstancedQuestObjective>*>(reinterpret_cast<std::uintptr_t>(a_player) + offset);
	}

	namespace compat
	{
		namespace AlternatePerspective
		{
			inline bool IsInstalled()
			{
				static bool s_checked = false;
				static bool s_installed = false;
				if (!s_checked) {
					s_checked = true;
					if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
						const auto* mod = dataHandler->LookupLoadedModByName("AlternatePerspective.esp");
						s_installed = (mod != nullptr);
						if (s_installed) {
							SKSE::log::info("Alternate Perspective detected: automatic start room quest cleanup enabled.");
						}
					}
				}
				return s_installed;
			}

			inline RE::TESQuest* GetStartCellQuest()
			{
				static RE::TESQuest* s_quest = nullptr;
				if (!s_quest && IsInstalled()) {
					if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
						s_quest = dataHandler->LookupForm<RE::TESQuest>(0x4246F9, "AlternatePerspective.esp");
					}
				}
				return s_quest;
			}

			inline RE::TESObjectCELL* GetStartCell()
			{
				static RE::TESObjectCELL* s_cell = nullptr;
				if (!s_cell && IsInstalled()) {
					if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
						s_cell = dataHandler->LookupForm<RE::TESObjectCELL>(0x2C1D00, "AlternatePerspective.esp");
					}
				}
				return s_cell;
			}

			inline void CleanUpLingeringQuest(RE::PlayerCharacter* a_player, RE::BSTArray<RE::BGSInstancedQuestObjective>& a_playerObjectives)
			{
				if (!IsInstalled() || !a_player) {
					return;
				}

				auto* startCellQ = GetStartCellQuest();
				if (!startCellQ || !startCellQ->IsRunning()) {
					return;
				}

				auto* currentCell = a_player->GetParentCell();
				auto* startCell = GetStartCell();

				// If player has left the starting room, shut down the quest cleanly
				if (currentCell && currentCell != startCell) {
					for (auto& obj : a_playerObjectives) {
						if (obj.objective && obj.objective->ownerQuest == startCellQ) {
							obj.instanceState = RE::QUEST_OBJECTIVE_STATE::kCompleted;
						}
					}
					startCellQ->Stop();
					if (auto storyTeller = RE::BGSStoryTeller::GetSingleton()) {
						storyTeller->BeginShutDownQuest(startCellQ);
					}
					SKSE::log::info("Alternate Perspective: Cleaned up lingering start room quest (AP_StartCellQ) because player left start cell.");
				}
			}

			inline bool IsStartCellTarget(RE::TESQuestTarget* a_questTarget, RE::PlayerCharacter* a_player)
			{
				if (!IsInstalled() || !a_questTarget || !a_questTarget->unk00 || !a_player) {
					return false;
				}

				auto* startCellQ = GetStartCellQuest();
				if (!startCellQ) {
					return false;
				}

				auto* currentCell = a_player->GetParentCell();
				auto* startCell = GetStartCell();
				if (currentCell && currentCell == startCell) {
					return false; // Still in start cell: do not suppress
				}

				auto questObjectiveTarget = reinterpret_cast<RE::TESQuestTarget*>(a_questTarget->unk00);
				for (auto* obj : startCellQ->objectives) {
					if (obj && obj->targets) {
						for (int j = 0; j < obj->numTargets; j++) {
							if (obj->targets[j] == questObjectiveTarget) {
								return true;
							}
						}
					}
				}

				return false;
			}
		}
	}

	bool UpdateQuests(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
					  RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame,
					  RE::TESQuestTarget* a_questTarget)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (player)
		{
			auto& playerObjectives = GetPlayerObjectives(player);
			compat::AlternatePerspective::CleanUpLingeringQuest(player, playerObjectives);

			// If this target belongs to Alternate Perspective's start room quest and player is outside, do not display marker
			if (compat::AlternatePerspective::IsStartCellTarget(a_questTarget, player))
			{
				return false;
			}
		}

		// `HUDMarkerManager::AddMarker` is also called iteratively for the same
		// previously-created marker (via `a_refHandle`), so we can iteratively
		// build the structure containing all the targets, objectives, etc. corresponding
		// to the marker.
		if (HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame))
		{
			RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

			if (!player || !a_questTarget || !a_questTarget->unk00)
			{
				return true;
			}

			auto& playerObjectives = GetPlayerObjectives(player);

			// The objectives are in oldest-to-newest order, so we iterate from newest-to-oldest
			// to have it in the same order as in the journal
			for (int ageIndex = static_cast<int>(playerObjectives.size()) - 1; ageIndex >= 0; ageIndex--)
			{
				RE::BGSInstancedQuestObjective* playerObjective = &playerObjectives[ageIndex];
				if (!playerObjective)
				{
					continue;
				}

				// Only process actively displayed objectives!
				if (playerObjective->instanceState != RE::QUEST_OBJECTIVE_STATE::kDisplayed)
				{
					continue;
				}

				RE::BGSQuestObjective* questObjective = playerObjective->objective;
				if (!questObjective || !questObjective->targets)
				{
					continue;
				}

				RE::TESQuest* quest = questObjective->ownerQuest;
				if (!quest || !quest->IsRunning())
				{
					continue;
				}

				for (int j = 0; j < questObjective->numTargets; j++)
				{
					if (!questObjective->targets[j])
					{
						continue;
					}

					auto questObjectiveTarget = reinterpret_cast<RE::TESQuestTarget*>(a_questTarget->unk00);

					if (questObjectiveTarget == questObjective->targets[j])
					{
						CNO::HUDMarkerManager::GetSingleton()->ProcessQuestMarker(quest, playerObjective, ageIndex,
																				  marker, a_markerGotoFrame);
						return true;
					}
				}
			}

			return true;
		}

		return false;
	}

	RE::TESWorldSpace* AllowedToShowMapMarker(const RE::TESObjectREFR* a_marker)
	{
		RE::TESWorldSpace* markerWorldspace = a_marker->GetWorldspace();

		if (settings::display::showInteriorMarkers)
		{
			auto player = RE::PlayerCharacter::GetSingleton();

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
		RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

		RE::NiPoint3 markerPos = util::GetRealPosition(marker);
		RE::NiPoint3 playerPos = util::GetRealPosition(player);

		float sqDistanceToMarker = playerPos.GetSquaredDistance(markerPos);

		if (sqDistanceToMarker < RE::HUDMarkerManager::GetSingleton()->sqRadiusToAddLocation)
		{
			auto mapMarker = marker->extraList.GetByType<RE::ExtraMapMarker>();

			// Unvisited markers keep being shown in any case
			if (settings::display::showUndiscoveredLocationMarkers || mapMarker->mapData->flags.all(RE::MapMarkerData::Flag::kVisible))
			{
				if (HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame)) 
				{
					CNO::HUDMarkerManager::GetSingleton()->ProcessLocationMarker(mapMarker, marker, a_markerGotoFrame);

					return true;
				}
			}
		}

		return false;
	}

	bool UpdateEnemies(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
							RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		if (settings::display::showEnemyMarkers)
		{
			if (HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame)) 
			{
				RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

				CNO::HUDMarkerManager::GetSingleton()->ProcessEnemyMarker(marker->As<RE::Character>(), a_markerGotoFrame);

				return true;
			}
		}

		return false;
	}

	bool UpdatePlayerSetMarker(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
								RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		if (HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame)) 
		{
			RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

			CNO::HUDMarkerManager::GetSingleton()->ProcessPlayerSetMarker(marker, a_markerGotoFrame);

			return true;
		}

		return false;
	}

	void UpdateCompass(RE::Compass* a_compass)
	{
		hooks::Compass::Update(a_compass);

		CNO::HUDMarkerManager::GetSingleton()->SetMarkersExtraInfo();
	}

	namespace compat
	{
		RE::GFxMovieDef* MapMarkerFramework::GetCompassMovieDef()
		{
			return compassMovieDef;
		}
	}
}
