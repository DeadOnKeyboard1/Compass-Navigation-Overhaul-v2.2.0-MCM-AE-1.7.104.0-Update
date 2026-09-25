#pragma once

#include "IUI/GFxArray.h"
#include "IUI/GFxDisplayObject.h"
#include "HUDDiscovery.h"

#include "Settings.h"
#include "utils/QuestText.h"

struct QuestItem
{
	QuestItem() = default;

	QuestItem(RE::TESObjectREFR* a_markerRef, RE::QUEST_DATA::Type a_questType, const std::string& a_questName,
			  bool a_isInSameLocation, int a_questAgeIndex)
	: markerRef{ a_markerRef }, type{ a_questType }, name{ a_questName }, isInSameLocation{ a_isInSameLocation },
	  ageIndex{ a_questAgeIndex }
	{}

	RE::TESObjectREFR* markerRef;
	RE::QUEST_DATA::Type type;
	std::string name;
	bool isInSameLocation;
	std::vector<RE::BGSInstancedQuestObjective*> objectives;
	int ageIndex;
};

class QuestItemList : public IUI::GFxDisplayObject
{
public:
	static constexpr inline std::string_view path = "_level0.HUDMovieBaseInstance.QuestItemList";

	static void InitSingleton(const GFxDisplayObject& a_questItemList, std::string_view a_path = {}, bool a_initialize = true)
	{
		if (!singleton) {
			static QuestItemList singletonInstance{ a_questItemList, a_initialize };
			singleton = &singletonInstance;
			if (!a_path.empty()) { singleton->activePath = a_path; }
		} else {
			*static_cast<GFxDisplayObject*>(singleton) = a_questItemList;
			singleton->hiddenByForce = false;
			if (a_initialize) { singleton->InitializeInstance(); }
			if (!a_path.empty()) { singleton->activePath = a_path; }
		}
	}

	static QuestItemList* GetSingleton() { return singleton; }

	static void InvalidateSingleton()
	{
		if (singleton) {
			singleton->Invalidate();
			singleton->hiddenByForce = false;
		}
	}

	[[nodiscard]] bool IsReady() const noexcept { return IsUsable(); }

	[[nodiscard]] const std::string& GetActivePath() const noexcept { return activePath; }

	bool CanBeDisplayed(RE::TESObjectCELL* a_cell, bool a_isPlayerInCombat) const
	{
		if (!a_isPlayerInCombat || !settings::questlist::hideInCombat)
		{
			if (a_cell)
			{
				if ((a_cell->IsInteriorCell() && settings::questlist::showInInteriors) ||
					(a_cell->IsExteriorCell() && settings::questlist::showInExteriors))
				{
					return true;
				}
			}
		}

		return false;
	}

	void SetHiddenByForce(bool a_hiddenByForce) { hiddenByForce = a_hiddenByForce; }

	bool IsHiddenByForce() const { return hiddenByForce; }

	void AddToHudElements()
	{
		if (HasMember("AddToHudElements")) {
			Invoke("AddToHudElements");
			return;
		}

		// Fallback for a compatible quest-list overlay whose helper function was
		// stripped by a UI replacer: register the live object directly in HudElements.
		auto hud = CNO::HUDDiscovery::FindHUDRoot(GetMovieView(), this);
		if (!hud) {
			return;
		}
		RE::GFxValue elements;
		if (static_cast<RE::GFxValue&>(*hud).GetMember("HudElements", &elements) && elements.IsArray()) {
			GFxArray hudElements{ elements, GetMovieView() };
			if (hudElements.IsUsable() && hudElements.FindElement(*this) < 0) {
				hudElements.PushBack(*this);
			}
		}
	}

	void AddQuest(const QuestItem& a_questItem)
	{
		auto* movieView = GetMovieView();
		if (!IsReady() || !movieView) {
			return;
		}

		GFxArray gfxQuestObjectives{ movieView };
		if (!gfxQuestObjectives.IsUsable()) {
			return;
		}
		std::vector<std::string> addedTexts;

		for (const RE::BGSInstancedQuestObjective* questObjective : a_questItem.objectives)
		{
			if (questObjective)
			{
				std::string text = util::GetObjectiveDisplayText(questObjective);
				if (!text.empty() && std::ranges::find(addedTexts, text) == addedTexts.end())
				{
					addedTexts.push_back(text);
					gfxQuestObjectives.PushBack(text.c_str());
				}
			}
		}

		if (HasMember("AddQuest")) {
			Invoke("AddQuest", a_questItem.type, a_questItem.name.c_str(), a_questItem.isInSameLocation,
				gfxQuestObjectives, a_questItem.ageIndex);
		}
	}

	void SetQuestSide(const std::string& a_sideName)
	{
		if (HasMember("SetQuestSide")) { Invoke("SetQuestSide", a_sideName.c_str()); }
	}

	void Update()
	{
		if (HasMember("Update")) { Invoke("Update"); }
	}

	void ShowQuest()
	{
		if (HasMember("ShowQuest")) { Invoke("ShowQuest"); }
	}

	void RemoveQuest()
	{
		if (HasMember("RemoveQuest")) { Invoke("RemoveQuest"); }
	}

	void ShowAllQuests()
	{
		if (HasMember("ShowAllQuests")) { Invoke("ShowAllQuests"); }
	}

	void RemoveAllQuests()
	{
		if (HasMember("RemoveAllQuests")) { Invoke("RemoveAllQuests"); }
	}

	void UpdateLayout()
	{
		auto movieView = GetMovieView();
		if (!movieView || !IsReady())
		{
			logger::warn("UpdateLayout: QuestItemList movieView or object invalid");
			return;
		}

		RE::GFxValue stageWidthVal, stageHeightVal;
		movieView->GetVariable(&stageWidthVal, "Stage.width");
		movieView->GetVariable(&stageHeightVal, "Stage.height");
		const double rawStageW = stageWidthVal.IsNumber() ? stageWidthVal.GetNumber() : 0.0;
		const double rawStageH = stageHeightVal.IsNumber() ? stageHeightVal.GetNumber() : 0.0;
		float stageW = (std::isfinite(rawStageW) && rawStageW > 0.0) ? static_cast<float>(rawStageW) : 1280.0F;
		float stageH = (std::isfinite(rawStageH) && rawStageH > 0.0) ? static_cast<float>(rawStageH) : 720.0F;

		float posX0 = stageW * settings::questlist::positionX;
		float posY0 = stageH * settings::questlist::positionY;
		float newMaxHeight = stageH * settings::questlist::maxHeight;

		if (!std::isfinite(posX0) || !std::isfinite(posY0) || !std::isfinite(newMaxHeight)) {
			logger::warn("QuestItemList UpdateLayout produced non-finite coordinates; leaving the active HUD transform untouched");
			return;
		}

		SetMember("positionX0", posX0);
		SetMember("positionY0", posY0);
		SetMember("maxHeight", newMaxHeight);

		IUI::GFxObject pt(movieView);
		pt.SetMember("x", posX0);
		pt.SetMember("y", posY0);

		float localX = posX0;
		float localY = posY0;

		RE::GFxValue parentObj = GetMember("_parent");
		bool foundParent = parentObj.IsDisplayObject();
		if (!foundParent) {
			if (auto hud = CNO::HUDDiscovery::FindHUDRoot(movieView, this)) {
				parentObj = *hud;
				foundParent = true;
			}
		}

		if (foundParent && parentObj.HasMember("globalToLocal"))
		{
			std::array<RE::GFxValue, 1> args{ pt };
			parentObj.Invoke("globalToLocal", nullptr, args.data(), 1);
			RE::GFxValue xVal = pt.GetMember("x");
			RE::GFxValue yVal = pt.GetMember("y");
			if (xVal.IsNumber() && yVal.IsNumber() &&
				std::isfinite(xVal.GetNumber()) && std::isfinite(yVal.GetNumber()))
			{
				localX = static_cast<float>(xVal.GetNumber());
				localY = static_cast<float>(yVal.GetNumber());
			}
		}

		SetMember("_x", localX);
		SetMember("_y", localY);
		SetMember("_xscale", settings::questlist::scale);
		SetMember("_yscale", settings::questlist::scale);

		logger::info("QuestItemList UpdateLayout: Stage({:.0f}x{:.0f}) pos({:.3f},{:.3f}) -> global({:.1f},{:.1f}) local({:.1f},{:.1f}) scale={:.1f}%",
			stageW, stageH, settings::questlist::positionX, settings::questlist::positionY,
			posX0, posY0, localX, localY, settings::questlist::scale);

		Update();
	}

private:

	QuestItemList(const GFxDisplayObject& a_questItemList, bool a_initialize) :
		GFxDisplayObject{ a_questItemList }
	{
		if (a_initialize) { InitializeInstance(); }
	}

	void InitializeInstance()
	{
		if (!IsReady()) {
			return;
		}
		if (HasMember("QuestItemList")) {
			Invoke("QuestItemList", settings::questlist::positionX, settings::questlist::positionY, settings::questlist::maxHeight);
		}
		SetMember("_xscale", settings::questlist::scale);
		SetMember("_yscale", settings::questlist::scale);
	}

	static inline QuestItemList* singleton = nullptr;
	std::string activePath{ path };

	bool hiddenByForce = false;
};