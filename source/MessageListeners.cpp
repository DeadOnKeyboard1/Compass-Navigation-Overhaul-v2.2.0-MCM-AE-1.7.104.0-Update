#include "Settings.h"

#include "REX/W32.h"

#include "IUI/API.h"
#include "NND/NPCNameProvider.h"

#include "Compass.h"
#include "HUDDiscovery.h"
#include "HUDMarkerManager.h"
#include "QuestItemList.h"
#include "Test.h"

#include "Hooks.h"

#undef GetModuleHandle

const SKSE::LoadInterface* skse;

void InfinityUIMessageListener(SKSE::MessagingInterface::Message* a_msg);

namespace
{
	std::optional<CNO::HUDDiscovery::LocatedDisplayObject> ResolveCompass(RE::GFxMovieView* a_movie)
	{
		return CNO::HUDDiscovery::FindCompass(a_movie);
	}

	std::optional<CNO::HUDDiscovery::LocatedDisplayObject> ResolveQuestItemList(RE::GFxMovieView* a_movie)
	{
		return CNO::HUDDiscovery::FindQuestItemList(a_movie);
	}

	void LogCompatibilityReport(RE::GFxMovieView* a_movie, std::string_view a_movieUrl)
	{
		auto* compass = CNO::Compass::GetSingleton();
		auto* questList = QuestItemList::GetSingleton();
		const bool compassReady = compass && compass->IsReady();
		const bool questListReady = questList && questList->IsReady();
		const int compassScore = compassReady ?
			CNO::HUDDiscovery::CompassScore(static_cast<const RE::GFxValue&>(*compass)) : 0;
		const int questScore = questListReady ?
			CNO::HUDDiscovery::QuestItemListScore(static_cast<const RE::GFxValue&>(*questList)) : 0;
		const bool fullCNOApi = compassReady && compass->SupportsCNOFunctions();

		bool hudRootFound = false;
		if (compassReady) {
			hudRootFound = CNO::HUDDiscovery::FindHUDRoot(a_movie, compass).has_value();
		} else {
			hudRootFound = CNO::HUDDiscovery::FindHUDRoot(a_movie).has_value();
		}

		std::string_view mode = !compassReady ? "no-compass" : (fullCNOApi ? "full" : "layout-only");
		logger::info(
			"CNO Compatibility Report: movie='{}', mode={}, compass='{}' score={}, fullCNOAPI={}, questList='{}' score={}, HUDRoot={}, InfinityUI=yes, CoMAPCompat={}",
			a_movieUrl,
			mode,
			compassReady ? compass->GetActivePath() : std::string("<none>"),
			compassScore,
			fullCNOApi ? "yes" : "no",
			questListReady ? questList->GetActivePath() : std::string("<none>"),
			questScore,
			hudRootFound ? "yes" : "no",
			hooks::compat::MapMarkerFramework::pluginInfo ? "yes" : "no");

		if (!compassReady) {
			return;
		}

		RE::GFxValue holder = compass->GetMember("_parent");
		if (!holder.IsDisplayObject()) {
			logger::info("CNO Compatibility Report: native holder baseline unavailable (no display-object parent)");
			return;
		}

		RE::GFxValue x, y, sx, sy;
		const bool hasBaseline =
			holder.GetMember("__CNO_BaseX", &x) && x.IsNumber() && std::isfinite(x.GetNumber()) &&
			holder.GetMember("__CNO_BaseY", &y) && y.IsNumber() && std::isfinite(y.GetNumber()) &&
			holder.GetMember("__CNO_BaseScaleX", &sx) && sx.IsNumber() && std::isfinite(sx.GetNumber()) &&
			holder.GetMember("__CNO_BaseScaleY", &sy) && sy.IsNumber() && std::isfinite(sy.GetNumber());
		if (hasBaseline) {
			logger::info("CNO Compatibility Report: native holder baseline pos({:.1f},{:.1f}) scale=({:.1f}%, {:.1f}%)",
				x.GetNumber(), y.GetNumber(), sx.GetNumber(), sy.GetNumber());
		} else {
			logger::info("CNO Compatibility Report: native holder baseline not stored");
		}
	}
}


void TriggerLayoutRefresh()
{
	settings::Reload();
	if (auto questList = QuestItemList::GetSingleton(); questList && questList->IsReady())
	{
		questList->UpdateLayout();
	}
	if (auto compass = CNO::Compass::GetSingleton(); compass && compass->IsReady())
	{
		compass->UpdateLayout();
	}
}

class MenuOpenCloseHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	static MenuOpenCloseHandler* GetSingleton()
	{
		static MenuOpenCloseHandler singleton;
		return &singleton;
	}

	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
	{
		if (!a_event)
		{
			return RE::BSEventNotifyControl::kContinue;
		}

		logger::debug("MenuOpenCloseEvent: menuName={}, opening={}", a_event->menuName.c_str(), a_event->opening);

		if (!a_event->opening && (a_event->menuName == RE::JournalMenu::MENU_NAME || a_event->menuName == "Journal Menu"))
		{
			logger::info("Journal menu closed - reloading settings and updating UI layout...");

			// MCM Helper can finish writing its INI at menu close. Prefer one next-frame
			// refresh so we read the final file exactly once; fall back to an immediate
			// refresh only if SKSE's task interface is unavailable.
			if (auto task = SKSE::GetTaskInterface())
			{
				task->AddTask([]() {
					TriggerLayoutRefresh();
				});
			}
			else
			{
				TriggerLayoutRefresh();
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}
};

void RegisterMenuObserver()
{
	static std::atomic_bool registered = false;
	if (!registered)
	{
		if (auto ui = RE::UI::GetSingleton())
		{
			ui->AddEventSink<RE::MenuOpenCloseEvent>(MenuOpenCloseHandler::GetSingleton());
			registered = true;
			logger::info("Registered MenuOpenCloseEvent sink for live MCM layout updates!");
		}
	}
}

void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg)
{
	if (!a_msg)
	{
		return;
	}

	if (a_msg->type == SKSE::MessagingInterface::kDataLoaded)
	{
		hooks::compat::AlternatePerspective::OnDataLoaded();
		RegisterMenuObserver();
		TriggerLayoutRefresh();
	}
	else if (a_msg->type == SKSE::MessagingInterface::kPreLoadGame)
	{
		// Drop raw quest/marker references before the old save is torn down.
		CNO::HUDMarkerManager::GetSingleton()->ResetRuntimeState("pre-load game");
	}
	else if (a_msg->type == SKSE::MessagingInterface::kPostLoadGame || a_msg->type == SKSE::MessagingInterface::kNewGame)
	{
		CNO::HUDMarkerManager::GetSingleton()->ResetRuntimeState(
			a_msg->type == SKSE::MessagingInterface::kNewGame ? "new game" : "post-load game");
		RegisterMenuObserver();
		TriggerLayoutRefresh();
	}

	// If all plugins have been loaded
	if (a_msg->type == SKSE::MessagingInterface::kPostLoad) 
	{
		if (SKSE::GetMessagingInterface()->RegisterListener("InfinityUI", InfinityUIMessageListener)) 
		{
			logger::info("Successfully registered for Infinity UI messages!");
		}
		else 
		{
			logger::error("Infinity UI installation not detected. Please, download it from https://www.nexusmods.com/skyrimspecialedition/mods/74483");
		}

		NND::NPCNameProvider::GetSingleton()->RequestAPI();

		const SKSE::PluginInfo* mapMarkerFrameworkPluginInfo = skse->GetPluginInfo("MapMarkerFramework");

		if (mapMarkerFrameworkPluginInfo && mapMarkerFrameworkPluginInfo->version < 0x02020000)
		{
			logger::info("CoMAP detected. Loading compatibility patch...");
			if (hooks::compat::MapMarkerFramework::Install(REX::W32::GetModuleHandleA("MapMarkerFramework.dll"))) {
				hooks::compat::MapMarkerFramework::pluginInfo = mapMarkerFrameworkPluginInfo;
				logger::info("Successfully loaded compatibility patch for CoMAP!");
			} else {
				logger::warn("CoMAP compatibility patch was not installed; CNO will continue without that integration");
			}
		}
	}
}

void InfinityUIMessageListener(SKSE::MessagingInterface::Message* a_msg)
{
	if (!a_msg || !a_msg->sender || std::string_view(a_msg->sender) != "InfinityUI")
	{
		return;
	}

	if (auto message = IUI::API::TranslateAs<IUI::API::Message>(a_msg)) 
	{
		if (!message->movie) {
			logger::warn("InfinityUI message did not provide a movie; ignoring it");
			return;
		}
		auto* movieDef = message->movie->GetMovieDef();
		if (!movieDef || !movieDef->GetFileURL()) {
			logger::warn("InfinityUI message movie has no movie definition/URL; ignoring it");
			return;
		}
		std::string_view movieUrl = movieDef->GetFileURL();

		bool isHUDMovie = movieUrl.find("HUDMenu") != std::string::npos;
		if (!isHUDMovie) {
			if (auto* ui = RE::UI::GetSingleton()) {
				if (auto hudMenu = ui->GetMenu<RE::HUDMenu>(); hudMenu && hudMenu->uiMovie.get() == message->movie) {
					isHUDMovie = true;
				}
			}
		}
		if (!isHUDMovie) {
			return;
		}

		switch (a_msg->type)
		{
		case IUI::API::Message::Type::kStartLoadInstances:
			// Reset marker/focus state before releasing the old HUD binding. The reset
			// touches only C++ runtime state and does not invoke Scaleform during patching.
			CNO::HUDMarkerManager::GetSingleton()->ResetRuntimeState("InfinityUI HUD rebuild");
			// Release managed Scaleform values while the old HUD movie is still alive.
			CNO::Compass::InvalidateSingleton();
			QuestItemList::InvalidateSingleton();
			Test::InvalidateSingleton();
			hooks::compat::MapMarkerFramework::compassMovieDef = nullptr;
			logger::info("Started loading HUD patches; invalidated cached Scaleform instances");
			break;
		case IUI::API::Message::Type::kPreReplaceInstance:
			if (auto preReplaceMessage = IUI::API::TranslateAs<IUI::API::PreReplaceInstanceMessage>(a_msg);
				preReplaceMessage && preReplaceMessage->originalInstance.IsDisplayObject() &&
				CNO::HUDDiscovery::LooksLikeCompass(preReplaceMessage->originalInstance))
			{
				IUI::GFxDisplayObject original{ preReplaceMessage->originalInstance, message->movie };
				CNO::Compass::InitSingleton(original, "<InfinityUI-pre-replace>");
				if (auto compass = CNO::Compass::GetSingleton(); compass && compass->IsReady()) {
					logger::debug("Before replacing runtime compass (capability score={})",
						CNO::HUDDiscovery::CompassScore(preReplaceMessage->originalInstance));
				}
			}
			break;
		case IUI::API::Message::Type::kPostPatchInstance:
			if (auto postPatchMessage = IUI::API::TranslateAs<IUI::API::PostPatchInstanceMessage>(a_msg);
				postPatchMessage && postPatchMessage->newInstance.IsDisplayObject())
			{
				const int compassScore = CNO::HUDDiscovery::CompassScore(postPatchMessage->newInstance);
				const int questScore = CNO::HUDDiscovery::QuestItemListScore(postPatchMessage->newInstance);
				const bool isCompass = compassScore >= 70 && compassScore >= questScore;
				const bool isQuestItemList = questScore >= 120 && questScore > compassScore;
				if (!isCompass && !isQuestItemList) {
					break;
				}

				IUI::GFxDisplayObject newInstance{ postPatchMessage->newInstance, message->movie };
				if (!newInstance.IsUsable()) {
					logger::warn("InfinityUI returned an unusable CNO display object");
					break;
				}

				if (isCompass)
				{
					CNO::Compass::InitSingleton(newInstance, "<InfinityUI-post-patch>");
					if (auto compass = CNO::Compass::GetSingleton())
					{
						// Do not invoke or introspect the replacement while InfinityUI is still
						// mutating the HUD tree. FinishLoadInstances re-resolves the final object
						// and performs SetupMod/layout after the patch batch is complete.
						logger::debug("Bound post-patch compass candidate (score={})", compassScore);

						if (hooks::compat::MapMarkerFramework::pluginInfo) {
							// CoMAP needs the movie definition that owns the active compass symbols.
							if (postPatchMessage->newInstanceMovieDef) {
								hooks::compat::MapMarkerFramework::compassMovieDef = postPatchMessage->newInstanceMovieDef;
							} else {
								logger::warn("CoMAP compatibility: active compass movie definition was not provided; preserving fallback behaviour");
							}
						}
					}
				}
				else if (isQuestItemList)
				{
					QuestItemList::InitSingleton(newInstance, "<InfinityUI-post-patch>", false);
					if (auto questItemList = QuestItemList::GetSingleton(); questItemList && questItemList->IsReady()) {
						// Initialization and layout are intentionally deferred until FinishLoadInstances.
						logger::debug("Bound post-patch quest-list candidate (score={})", questScore);
					}
				}
			}
			break;
		case IUI::API::Message::Type::kAbortPatchInstance:
			if (auto abortPatchMessage = IUI::API::TranslateAs<IUI::API::AbortPatchInstanceMessage>(a_msg);
				abortPatchMessage && abortPatchMessage->originalValue.IsDisplayObject())
			{
				if (CNO::HUDDiscovery::LooksLikeCompass(abortPatchMessage->originalValue)) {
					CNO::Compass::InvalidateSingleton();
					hooks::compat::MapMarkerFramework::compassMovieDef = nullptr;
					logger::error("Aborted replacement of a runtime compass candidate (score={})",
						CNO::HUDDiscovery::CompassScore(abortPatchMessage->originalValue));
				} else if (CNO::HUDDiscovery::LooksLikeQuestItemList(abortPatchMessage->originalValue)) {
					QuestItemList::InvalidateSingleton();
					logger::error("Aborted replacement of a runtime quest-list candidate (score={})",
						CNO::HUDDiscovery::QuestItemListScore(abortPatchMessage->originalValue));
				}
			}
			break;
		case IUI::API::Message::Type::kFinishLoadInstances:
			if (auto finishLoadMessage = IUI::API::TranslateAs<IUI::API::FinishLoadInstancesMessage>(a_msg);
				finishLoadMessage && finishLoadMessage->movie)
			{
				RE::GFxValue test;
				if (finishLoadMessage->movie->GetVariable(&test, Test::path.data()) && test.IsDisplayObject()) {
					Test::InitSingleton(IUI::GFxDisplayObject{ test, finishLoadMessage->movie });
				}

				// Re-resolve final HUD objects by behaviour/signature after all replacements.
				// This supports renamed/reparented compass holders used by UI skins.
				if (auto finalCompass = ResolveCompass(finishLoadMessage->movie)) {
					CNO::Compass::InitSingleton(finalCompass->object, finalCompass->path);
					if (auto compass = CNO::Compass::GetSingleton(); compass && compass->IsReady()) {
						compass->SetupMod(finalCompass->object);
						logger::info("Universal HUD detection bound compass '{}' (score={})", finalCompass->path, finalCompass->score);
					}
				} else {
					CNO::Compass::InvalidateSingleton();
					hooks::compat::MapMarkerFramework::compassMovieDef = nullptr;
					logger::warn("Universal HUD detection could not identify a compass instance; CNO will leave this HUD untouched");
				}

				if (auto finalQuestList = ResolveQuestItemList(finishLoadMessage->movie)) {
					QuestItemList::InitSingleton(finalQuestList->object, finalQuestList->path);
					logger::info("Universal HUD detection bound quest list '{}' (score={})", finalQuestList->path, finalQuestList->score);
				} else {
					QuestItemList::InvalidateSingleton();
					logger::info("No CNO quest-list overlay detected; focused compass functionality remains active without it");
				}

			}
			else
			{
				CNO::Compass::InvalidateSingleton();
				QuestItemList::InvalidateSingleton();
				Test::InvalidateSingleton();
				hooks::compat::MapMarkerFramework::compassMovieDef = nullptr;
				logger::warn("InfinityUI FinishLoadInstances message was incomplete; invalidated provisional HUD bindings");
				break;
			}
			RegisterMenuObserver();
			TriggerLayoutRefresh();
			LogCompatibilityReport(message->movie, movieUrl);
			logger::info("Finished loading HUD patches");
			break;
		case IUI::API::Message::Type::kPostInitExtensions:
			if (auto postInitExtMessage = IUI::API::TranslateAs<IUI::API::PostInitExtensionsMessage>(a_msg))
			{
				if (auto questItemList = QuestItemList::GetSingleton(); questItemList && questItemList->IsReady())
				{
					questItemList->AddToHudElements();

					logger::debug("QuestItemList added to HUD elements");
				}

				RegisterMenuObserver();
				TriggerLayoutRefresh();
				logger::debug("Extensions initialization finished");
			}
			break;
		default:
			break;
		}
	}
}
