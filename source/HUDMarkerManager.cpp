#include "HUDMarkerManager.h"

#include "RE/B/BSTimer.h"

#include "RE/I/IMenu.h"

#include "NND/NPCNameProvider.h"
#include "utils/QuestText.h"

namespace CNO
{
	void HUDMarkerManager::ProcessQuestMarker(RE::TESQuest* a_quest, RE::BGSInstancedQuestObjective* a_questObjective,
											  int a_questAgeIndex, RE::TESObjectREFR* a_marker, std::uint32_t a_markerIcon, std::uint32_t a_markerIndex)
	{
		if (!a_quest || !a_questObjective || !a_marker || !playerCamera) {
			return;
		}

		float angleToPlayerCamera = GetAngleBetween(playerCamera, a_marker);

		if ((IsTheFocusedMarker(a_marker) && angleToPlayerCamera < settings::display::angleToKeepMarkerDetailsShown) ||
			angleToPlayerCamera < settings::display::angleToShowMarkerDetails)
		{
			std::string description;

			if (settings::display::showObjectiveAsTarget)
			{
				description = util::GetObjectiveDisplayText(a_questObjective);
			}
			else
			{
				// A quest marker can reference to a character or a location
				switch (a_marker->GetFormType())
				{
				case RE::FormType::Reference:
					if (auto teleportDoor = a_marker->As<RE::TESObjectREFR>())
					{
						// If it is a teleport door, we can get the door at the other side
						if (auto teleportLinkedDoor = teleportDoor->extraList.GetTeleportLinkedDoor().get())
						{
							// First, try interior cell
							if (RE::TESObjectCELL* cell = teleportLinkedDoor->GetParentCell())
							{
								description = cell->GetName();
							}
							// Exterior cell
							else if (RE::TESWorldSpace* worldSpace = teleportLinkedDoor->GetWorldspace())
							{
								description = worldSpace->GetName();
							}
						}
					}
					break;
				case RE::FormType::ActorCharacter:
					if (auto character = a_marker->As<RE::Character>())
					{
						description = NND::NPCNameProvider::GetSingleton()->GetName(character);
					}
					break;
				}
			}

			auto it = std::ranges::find_if(facedMarkers, [&](const Compass::Marker& m) { return m.ref == a_marker; });
			if (it == facedMarkers.end())
			{
				facedMarkers.emplace_back(a_marker, angleToPlayerCamera,
										  a_markerIndex,
										  a_markerIcon, description);
			}

			RE::QUEST_DATA::Type questType = a_quest->GetType();

			auto* frameOffsets = RE::HUDMarker::FrameOffsets::GetSingleton();
			bool isInSameLocation = frameOffsets && a_markerIcon == frameOffsets->quest;

			QuestItem* questItem;

			if (questType == RE::QUEST_DATA::Type::kMiscellaneous)
			{
				auto [it, inserted] = miscQuestItem.try_emplace(
					a_marker, a_marker, questType, "$MISCELLANEOUS", isInSameLocation, a_questAgeIndex);
				questItem = &it->second;
				if (!inserted) {
					questItem->isInSameLocation = questItem->isInSameLocation || isInSameLocation;
					questItem->ageIndex = std::max(questItem->ageIndex, a_questAgeIndex);
				}
			}
			else
			{
				RE::BSString questFullName = a_quest->GetFullName();
				util::ReplaceTagsInQuestText(&questFullName, a_quest, a_quest->currentInstanceID);

				auto& markerQuests = questItems[a_marker];
				auto [it, inserted] = markerQuests.try_emplace(
					a_quest, a_marker, questType, questFullName.c_str(), isInSameLocation, a_questAgeIndex);
				questItem = &it->second;
				if (!inserted) {
					questItem->isInSameLocation = questItem->isInSameLocation || isInSameLocation;
					questItem->ageIndex = std::max(questItem->ageIndex, a_questAgeIndex);
				}
			}

			// The engine may call AddMarker more than once for the same instanced
			// objective when it has multiple targets. De-duplicate by the instanced
			// objective identity only; different radiant instances may share the same
			// base BGSQuestObjective and must not be collapsed.
			if (std::ranges::find(questItem->objectives, a_questObjective) == questItem->objectives.end())
			{
				questItem->objectives.push_back(a_questObjective);
			}
		}
	}

	void HUDMarkerManager::ProcessLocationMarker(RE::ExtraMapMarker* a_mapMarker, RE::TESObjectREFR* a_marker,
												 std::uint32_t a_markerIcon, std::uint32_t a_markerIndex, RE::HUDMarker::ScaleformData* a_markerData)
	{
		if (!a_mapMarker || !a_mapMarker->mapData || !a_marker || !playerCamera) {
			return;
		}

		float angleToPlayerCamera = GetAngleBetween(playerCamera, a_marker);

		bool isDiscoveredLocation = a_mapMarker->mapData->flags.all(RE::MapMarkerData::Flag::kVisible);

		if ((IsTheFocusedMarker(a_marker) && angleToPlayerCamera < settings::display::angleToKeepMarkerDetailsShown) ||
			angleToPlayerCamera < settings::display::angleToShowMarkerDetails)
		{
			// Keep undiscovered locations in the focus pipeline so their distance/height
			// can still be shown. Unknown-info mode hides only the name.
			std::string_view locationDescription{};
			if (isDiscoveredLocation || !settings::display::undiscoveredMeansUnknownInfo) {
				locationDescription = a_mapMarker->mapData->locationName.GetFullName();
			}

			facedMarkers.emplace_back(a_marker, angleToPlayerCamera,
								  a_markerIndex,
								  a_markerIcon, locationDescription);
		}

		if (!isDiscoveredLocation && settings::display::undiscoveredMeansUnknownMarkers)
		{
			// AddMarker has already copied the input ScaleformData into the manager buffer.
			// Update that engine-owned slot (the original CNO behaviour), not the input
			// temporary.  The explicit index check prevents an under/overflow write.
			if (auto* manager = RE::HUDMarkerManager::GetSingleton(); manager && a_markerIndex < 49) {
				manager->scaleformMarkerData[a_markerIndex].icon.SetNumber(0);
			} else if (a_markerData) {
				a_markerData->icon.SetNumber(0);
			}
		}
	}

	void HUDMarkerManager::ProcessEnemyMarker(RE::Character* a_enemy, std::uint32_t a_markerIcon, std::uint32_t a_markerIndex)
	{
		if (!a_enemy || !playerCamera) {
			return;
		}

		float angleToPlayerCamera = GetAngleBetween(playerCamera, a_enemy);

		if ((IsTheFocusedMarker(a_enemy) && angleToPlayerCamera < settings::display::angleToKeepMarkerDetailsShown) ||
			angleToPlayerCamera < settings::display::angleToShowMarkerDetails)
		{
			std::string enemyName = NND::NPCNameProvider::GetSingleton()->GetName(a_enemy);

			facedMarkers.emplace_back(a_enemy, angleToPlayerCamera,
									a_markerIndex,
									a_markerIcon, enemyName);
		}
	}

	void HUDMarkerManager::ProcessPlayerSetMarker(RE::TESObjectREFR* a_marker, std::uint32_t a_markerIcon, std::uint32_t a_markerIndex)
	{
		if (!a_marker || !playerCamera) {
			return;
		}

		float angleToPlayerCamera = GetAngleBetween(playerCamera, a_marker);

		if ((IsTheFocusedMarker(a_marker) && angleToPlayerCamera < settings::display::angleToKeepMarkerDetailsShown) ||
			angleToPlayerCamera < settings::display::angleToShowMarkerDetails)
		{
			facedMarkers.emplace_back(a_marker, angleToPlayerCamera,
										a_markerIndex,
										a_markerIcon, "");
		}
	}

	void HUDMarkerManager::SetMarkersExtraInfo()
	{
		// These are engine singletons, but refreshing the pointers here avoids
		// permanently caching null values if CNO is touched unusually early.
		player = RE::PlayerCharacter::GetSingleton();
		playerCamera = RE::PlayerCamera::GetSingleton();
		timeManager = RE::BSTimer::GetSingleton();

		auto* compass = Compass::GetSingleton();
		auto* questItemList = QuestItemList::GetSingleton();
		if (!compass || !compass->IsReady() || !questItemList || !questItemList->IsReady() || !player || !playerCamera || !timeManager) {
			// InfinityUI may not have created/replaced the HUD instances yet. Do not
			// retain stale marker data across frames while waiting for the UI.
			facedMarkers.clear();
			questItems.clear();
			miscQuestItem.clear();
			return;
		}

		bool focusChanged = UpdateFocusedMarker();

		if (focusChanged)
		{
			compass->UnfocusMarker();
			timeFocusingMarker = 0.0F;
		}
		else if (focusedMarker)
		{
			timeFocusingMarker += timeManager->realTimeDelta;
		}

		bool isFocusedQuestMarker = false;

		if (focusedMarker)
		{
			std::string focusedMarkerDescription = focusedMarker->description;

			const bool hasRegularQuests = questItems.contains(focusedMarker->ref);
			const bool hasMiscQuest = miscQuestItem.contains(focusedMarker->ref);
			isFocusedQuestMarker = hasRegularQuests || hasMiscQuest;

			if (settings::display::showObjectiveAsTarget && settings::display::showOtherObjectivesCount && isFocusedQuestMarker)
			{
				std::size_t objectivesCount = 0;
				if (hasRegularQuests) {
					for (const auto& [quest, questItem] : questItems[focusedMarker->ref]) {
						(void)quest;
						objectivesCount += questItem.objectives.size();
					}
				}
				if (hasMiscQuest) {
					objectivesCount += miscQuestItem[focusedMarker->ref].objectives.size();
				}
				if (objectivesCount > 1) {
					focusedMarkerDescription += " (+" + std::to_string(objectivesCount - 1) + ")";
				}
			}

			compass->SetFocusedMarkerInfo(focusedMarkerDescription, focusedMarker->distanceToPlayer,
										  focusedMarker->heightDifference, focusedMarker->index);

			if (focusChanged)
			{
				compass->FocusMarker(focusedMarker->index);
			}

			compass->UpdateFocusedMarker();
		}

		RE::ActorState* playerState = player->AsActorState();
		if (!playerState) {
			facedMarkers.clear();
			questItems.clear();
			miscQuestItem.clear();
			return;
		}

		bool canQuestItemListBeDisplayed = questItemList->CanBeDisplayed(player->GetParentCell(), player->IsInCombat());

		if (!canQuestItemListBeDisplayed || focusChanged)
		{
			questItemList->RemoveAllQuests();
		}

		if (canQuestItemListBeDisplayed && isFocusedQuestMarker)
		{
			if (focusChanged)
			{
				if (questItems.contains(focusedMarker->ref))
				{
					std::unordered_map<RE::TESQuest*, QuestItem>& questItemMap = questItems[focusedMarker->ref];

					for (auto& [quest, questItem] : questItemMap)
					{
						questItemList->AddQuest(questItem);
						questItemList->SetQuestSide(GetSideInQuest(questItem.type));

						// If we call a function more than once per frame (like in this for-loop)
						// we need to update the stage with `GFxMovieView::Advance`, otherwise graphical
						// glitches occur to the element when showing up
						if (auto* movieView = questItemList->GetMovieView()) {
							movieView->Advance(0.0F);
						}
					}
				}
				
				if (miscQuestItem.contains(focusedMarker->ref))
				{
					QuestItem& questItem = miscQuestItem[focusedMarker->ref];

					questItemList->AddQuest(questItem);

					// If we call a function more than once per frame (like in this for-loop)
					// we need to update the stage with `GFxMovieView::Advance`, otherwise graphical
					// glitches occur to the element when showing up
					if (auto* movieView = questItemList->GetMovieView()) {
						movieView->Advance(0.0F);
					}
				}
			}

			questItemList->SetHiddenByForce(false);

			float playerSpeed = playerState->DoGetMovementSpeed();

			float delayToShow = (playerSpeed < player->GetWalkSpeed()) ? settings::questlist::walkingDelayToShow :
								(playerSpeed < player->GetJogSpeed())  ? settings::questlist::joggingDelayToShow :
																		 settings::questlist::sprintingDelayToShow;

			if (timeFocusingMarker > delayToShow)
			{
				questItemList->ShowAllQuests();
				questItemList->Update();
			}
		}

		facedMarkers.clear();
		questItems.clear();
		miscQuestItem.clear();
	}

	std::unique_ptr<Compass::Marker> HUDMarkerManager::GetMostCenteredMarker() const
	{
		std::unique_ptr<Compass::Marker> mostCenteredMarker = nullptr;

		float closestAngleToPlayerCamera = (std::numeric_limits<float>::max)();

		int mostCenteredMarkerIndex = -1;

		for (int i = 0; i < facedMarkers.size(); i++)
		{
			const Compass::Marker& facedMarker = facedMarkers[i];

			if (facedMarker.angleToPlayerCamera < closestAngleToPlayerCamera)
			{
				mostCenteredMarkerIndex = i;
				closestAngleToPlayerCamera = facedMarker.angleToPlayerCamera;
			}
		}

		if (mostCenteredMarkerIndex >= 0)
		{
			mostCenteredMarker = std::make_unique<Compass::Marker>(facedMarkers[mostCenteredMarkerIndex]);
		}

		return mostCenteredMarker;
	}

	bool HUDMarkerManager::UpdateFocusedMarker()
	{
		std::unique_ptr<Compass::Marker> mostCenteredMarker = GetMostCenteredMarker();

		static auto IsMarkerDifferent = [](const std::unique_ptr<Compass::Marker>& a_lhs, const std::unique_ptr<Compass::Marker>& a_rhs) -> bool
		{
			if (a_lhs && a_rhs)
			{
				return a_lhs->ref != a_rhs->ref;
			}
			else if (!a_lhs && !a_rhs)
			{
				return false;
			}

			return true;
		};

		if (IsMarkerDifferent(mostCenteredMarker, preFocusedMarker))
		{
			timePreFocusingMarker = 0.0F;
		}

		if (preFocusedMarker || mostCenteredMarker)
		{
			preFocusedMarker = std::move(mostCenteredMarker);
		}

		if (IsMarkerDifferent(preFocusedMarker, focusedMarker))
		{
			if (preFocusedMarker)
			{
				if (timePreFocusingMarker > settings::display::focusingDelayToShow)
				{	
					focusedMarker = std::move(preFocusedMarker);
					return true;
				}
				else
				{
					timePreFocusingMarker += timeManager->realTimeDelta;
				}
			}
			else
			{
				focusedMarker = nullptr;
				return true;
			}
		}
		else if (preFocusedMarker && focusedMarker)
		{
			focusedMarker = std::move(preFocusedMarker);
		}

		return false;
	}

	float HUDMarkerManager::GetAngleBetween(const RE::PlayerCamera* a_playerCamera,
											const RE::TESObjectREFR* a_marker) const
	{
		float angleToPlayerCameraInRadians = util::GetAngleBetween(a_playerCamera, a_marker);
		float angleToPlayerCamera = util::RadiansToDegrees(angleToPlayerCameraInRadians);

		if (angleToPlayerCamera > 180.0F)
			angleToPlayerCamera = 360.0F - angleToPlayerCamera;

		return angleToPlayerCamera;
	}

	bool HUDMarkerManager::IsPlayerAllyOfFaction(const RE::TESFaction* a_faction) const
	{
		if (!player || !a_faction) {
			return false;
		}

		if (player->IsInFaction(a_faction)) 
		{
			return true;
		}

		return player->VisitFactions([a_faction](RE::TESFaction* a_visitedFaction, std::int8_t a_rank) -> bool
		{
			if (a_visitedFaction == a_faction && a_rank > 0)
			{
				return true;
			}

			if (!a_visitedFaction) {
				return false;
			}

			for (RE::GROUP_REACTION* reactionToFaction : a_visitedFaction->reactions)
			{
				if (!reactionToFaction || !reactionToFaction->form) {
					continue;
				}
				auto relatedFaction = reactionToFaction->form->As<RE::TESFaction>();
				if (relatedFaction == a_faction && reactionToFaction->fightReaction >= RE::FIGHT_REACTION::kAlly)
				{
					return true;
				}
			}

			return false;
		});
	}

	bool HUDMarkerManager::IsPlayerOpponentOfFaction(const RE::TESFaction* a_faction) const
	{
		if (!player || !a_faction) {
			return false;
		}

		return player->VisitFactions([a_faction](RE::TESFaction* a_visitedFaction, std::int8_t a_rank) -> bool
		{
			if (a_visitedFaction == a_faction && a_rank < 0)
			{
				return true;
			}

			if (!a_visitedFaction) {
				return false;
			}

			for (RE::GROUP_REACTION* reactionToFaction : a_visitedFaction->reactions)
			{
				if (!reactionToFaction || !reactionToFaction->form) {
					continue;
				}
				auto relatedFaction = reactionToFaction->form->As<RE::TESFaction>();
				if (relatedFaction == a_faction && reactionToFaction->fightReaction == RE::FIGHT_REACTION::kEnemy)
				{
					return true;
				}
			}

			return false;
		});
	}

	std::string HUDMarkerManager::GetSideInQuest(RE::QUEST_DATA::Type a_questType) const
	{
		if (!player) {
			return {};
		}

		// Resolve factions lazily. Constructing this singleton before kDataLoaded must
		// not permanently cache null pointers for Skyrim/Dawnguard forms.
		switch (a_questType)
		{
		case RE::QUEST_DATA::Type::kCivilWar: {
			const auto* imperialLegionFaction = LookupFaction(0x0002BF9A);
			const auto* stormCloaksFaction = LookupFaction(0x00028849);
			const auto* sonsOfSkyrimFaction = LookupFaction(0x0002BF9B);
			if (IsPlayerAllyOfFaction(sonsOfSkyrimFaction) || IsPlayerAllyOfFaction(stormCloaksFaction) ||
				IsPlayerOpponentOfFaction(imperialLegionFaction))
			{
				return "StormCloaks";
			}
			return "ImperialLegion";
		}
		case RE::QUEST_DATA::Type::kDLC01_Vampire: {
			const auto* dawnGuardFaction = LookupFaction(0x02014217);
			const auto* vampireFaction = LookupFaction(0x02003376);
			if (player->HasKeywordString("Vampire") || IsPlayerAllyOfFaction(vampireFaction) ||
				IsPlayerOpponentOfFaction(dawnGuardFaction))
			{
				return "Vampires";
			}
			return "Dawnguard";
		}
		default:
			break;
		}

		return {};
	}
}
