#pragma once

#include "IUI/GFxArray.h"
#include "IUI/GFxDisplayObject.h"

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

	static void InitSingleton(const GFxDisplayObject& a_questItemList)
	{
		if (!singleton) {
			static QuestItemList singletonInstance{ a_questItemList };
			singleton = &singletonInstance;
		} else {
			*static_cast<GFxDisplayObject*>(singleton) = a_questItemList;
			singleton->hiddenByForce = false;
			singleton->InitializeInstance();
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
		Invoke("AddToHudElements");
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

		Invoke("AddQuest", a_questItem.type, a_questItem.name.c_str(), a_questItem.isInSameLocation,
			   gfxQuestObjectives, a_questItem.ageIndex);
	}

	void SetQuestSide(const std::string& a_sideName)
	{
		Invoke("SetQuestSide", a_sideName.c_str());
	}

	void Update()
	{
		Invoke("Update");
	}

	void ShowQuest()
	{
		Invoke("ShowQuest");
	}

	void RemoveQuest()
	{
		Invoke("RemoveQuest");
	}

	void ShowAllQuests()
	{
		Invoke("ShowAllQuests");
	}

	void RemoveAllQuests()
	{
		Invoke("RemoveAllQuests");
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
		float stageW = (stageWidthVal.IsNumber() && stageWidthVal.GetNumber() > 0) ? static_cast<float>(stageWidthVal.GetNumber()) : 1280.0F;
		float stageH = (stageHeightVal.IsNumber() && stageHeightVal.GetNumber() > 0) ? static_cast<float>(stageHeightVal.GetNumber()) : 720.0F;

		float posX0 = stageW * settings::questlist::positionX;
		float posY0 = stageH * settings::questlist::positionY;
		float newMaxHeight = stageH * settings::questlist::maxHeight;

		SetMember("positionX0", posX0);
		SetMember("positionY0", posY0);
		SetMember("maxHeight", newMaxHeight);

		IUI::GFxObject pt(movieView);
		pt.SetMember("x", posX0);
		pt.SetMember("y", posY0);

		float localX = posX0;
		float localY = posY0;

		RE::GFxValue parentObj = GetMember("_parent");
		bool foundParent = false;
		if (parentObj.IsObject())
		{
			foundParent = true;
		}
		else if (movieView->GetVariable(&parentObj, "_level0.HUDMovieBaseInstance") && parentObj.IsObject())
		{
			foundParent = true;
		}

		if (foundParent)
		{
			std::array<RE::GFxValue, 1> args{ pt };
			parentObj.Invoke("globalToLocal", nullptr, args.data(), 1);
			RE::GFxValue xVal = pt.GetMember("x");
			RE::GFxValue yVal = pt.GetMember("y");
			if (xVal.IsNumber() && yVal.IsNumber())
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

	QuestItemList(const GFxDisplayObject& a_questItemList) :
		GFxDisplayObject{ a_questItemList }
	{
		InitializeInstance();
	}

	void InitializeInstance()
	{
		if (!IsReady()) {
			return;
		}
		Invoke("QuestItemList", settings::questlist::positionX, settings::questlist::positionY, settings::questlist::maxHeight);
		SetMember("_xscale", settings::questlist::scale);
		SetMember("_yscale", settings::questlist::scale);
	}

	static inline QuestItemList* singleton = nullptr;

	bool hiddenByForce = false;
};