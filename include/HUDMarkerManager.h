#pragma once

#include "RE/H/HUDMarkerManager.h"

#include "Settings.h"

#include "Compass.h"
#include "QuestItemList.h"

namespace CNO
{
	class HUDMarkerManager
	{
	public:
		static HUDMarkerManager* GetSingleton()
		{
			static HUDMarkerManager singleton;

			return &singleton;
		}

		void ProcessQuestMarker(RE::TESQuest* a_quest, RE::BGSInstancedQuestObjective* a_questObjective,
			int a_questAgeIndex, RE::TESObjectREFR* a_marker, std::uint32_t a_markerIcon, std::uint32_t a_markerIndex);

		void ProcessLocationMarker(RE::ExtraMapMarker* a_mapMarker, RE::TESObjectREFR* a_marker,
			std::uint32_t a_markerIcon, std::uint32_t a_markerIndex, RE::HUDMarker::ScaleformData* a_markerData);

		void ProcessEnemyMarker(RE::Character* a_enemy, std::uint32_t a_markerIcon, std::uint32_t a_markerIndex);

		void ProcessPlayerSetMarker(RE::TESObjectREFR* a_marker, std::uint32_t a_markerIcon, std::uint32_t a_markerIndex);

		void SetMarkersExtraInfo();

	private:

		bool IsTheFocusedMarker(const RE::TESObjectREFR* a_marker) const
		{
			return focusedMarker && a_marker == focusedMarker->ref;
		}

		std::unique_ptr<Compass::Marker> GetMostCenteredMarker() const;

		bool UpdateFocusedMarker();

		float GetAngleBetween(const RE::PlayerCamera* a_playerCamera, const RE::TESObjectREFR* a_marker) const;

		bool IsPlayerAllyOfFaction(const RE::TESFaction* a_faction) const;

		bool IsPlayerOpponentOfFaction(const RE::TESFaction* a_faction) const;

		std::string GetSideInQuest(RE::QUEST_DATA::Type a_questType) const;

		float timePreFocusingMarker = 0.0F;
		float timeFocusingMarker = 0.0F;

		std::vector<Compass::Marker> facedMarkers;
		std::unique_ptr<Compass::Marker> preFocusedMarker;
		std::unique_ptr<Compass::Marker> focusedMarker;

		std::unordered_map<RE::TESObjectREFR*, std::unordered_map<RE::TESQuest*, QuestItem>> questItems;
		std::unordered_map<RE::TESObjectREFR*, QuestItem> miscQuestItem;

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		RE::PlayerCamera* playerCamera = RE::PlayerCamera::GetSingleton();
		RE::BSTimer* timeManager = RE::BSTimer::GetSingleton();

		static const RE::TESFaction* LookupFaction(RE::FormID a_formID)
		{
			if (auto* form = RE::TESForm::LookupByID(a_formID)) {
				return form->As<RE::TESFaction>();
			}
			return nullptr;
		}

	};
}