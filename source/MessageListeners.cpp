#include "Settings.h"

#include "REX/W32.h"

#include "IUI/API.h"
#include "NND/NPCNameProvider.h"

#include "Compass.h"
#include "QuestItemList.h"
#include "Test.h"

#include "IUI/GFxLoggers.h"

#include "Hooks.h"

#undef GetModuleHandle

const SKSE::LoadInterface* skse;

void InfinityUIMessageListener(SKSE::MessagingInterface::Message* a_msg);

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
			TriggerLayoutRefresh();

			// Also schedule delayed refresh via task interface on next frame (in case MCMHelper wrote INI asynchronously)
			if (auto task = SKSE::GetTaskInterface())
			{
				task->AddTask([]() {
					TriggerLayoutRefresh();
				});
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
	else if (a_msg->type == SKSE::MessagingInterface::kPostLoadGame || a_msg->type == SKSE::MessagingInterface::kNewGame)
	{
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

		if (movieUrl.find("HUDMenu") == std::string::npos)
		{
			return;
		}

		GFxMemberLogger<logger::level::debug> memberLogger;

		switch (a_msg->type)
		{
		case IUI::API::Message::Type::kStartLoadInstances:
			// Release managed Scaleform values while the old HUD movie is still alive.
			CNO::Compass::InvalidateSingleton();
			QuestItemList::InvalidateSingleton();
			Test::InvalidateSingleton();
			hooks::compat::MapMarkerFramework::compassMovieDef = nullptr;
			logger::info("Started loading HUD patches; invalidated cached Scaleform instances");
			break;
		case IUI::API::Message::Type::kPreReplaceInstance:
			if (auto preReplaceMessage = IUI::API::TranslateAs<IUI::API::PreReplaceInstanceMessage>(a_msg);
				preReplaceMessage && preReplaceMessage->originalInstance.IsDisplayObject())
			{
				std::string pathToOriginal = preReplaceMessage->originalInstance.ToString().c_str();

				if (pathToOriginal == CNO::Compass::path)
				{
					IUI::GFxDisplayObject original{ preReplaceMessage->originalInstance, message->movie };
					if (!original.IsUsable()) {
						logger::warn("PreReplace compass instance is not a usable display object");
						break;
					}
					CNO::Compass::InitSingleton(original);
					auto compass = CNO::Compass::GetSingleton();

					logger::debug("Before replacing:");
					memberLogger.LogMembersOf(*compass);
					if (auto coord = compass->LocalToGlobal()) {
						logger::debug("{} is on ({}, {})", compass->ToString().c_str(), coord->x, coord->y);
					}
				}
			}
			break;
		case IUI::API::Message::Type::kPostPatchInstance:
			if (auto postPatchMessage = IUI::API::TranslateAs<IUI::API::PostPatchInstanceMessage>(a_msg);
				postPatchMessage && postPatchMessage->newInstance.IsDisplayObject())
			{
				std::string pathToNew = postPatchMessage->newInstance.ToString().c_str();
				IUI::GFxDisplayObject newInstance{ postPatchMessage->newInstance, message->movie };
				if (!newInstance.IsUsable()) {
					logger::warn("InfinityUI returned an unusable display object for {}", pathToNew);
					break;
				}

				if (pathToNew == CNO::Compass::path)
				{
					if (auto compass = CNO::Compass::GetSingleton())
					{
						compass->SetupMod(newInstance);
						compass->SetUnits(settings::display::useMetricUnits);
						compass->UpdateLayout();

						logger::debug("After replacing:");
						memberLogger.LogMembersOf(*compass);
						if (auto coord = compass->LocalToGlobal()) {
							logger::debug("{} is on ({}, {})", compass->ToString().c_str(), coord->x, coord->y);
						}

						if (hooks::compat::MapMarkerFramework::pluginInfo) {
							// CoMAP needs the movie definition that owns the replacement Compass
							// symbols.  The outer HUDMenu movie definition is NOT equivalent and
							// makes normal location/enemy/player marker assets disappear.
							if (postPatchMessage->newInstanceMovieDef) {
								hooks::compat::MapMarkerFramework::compassMovieDef = postPatchMessage->newInstanceMovieDef;
							} else {
								logger::warn("CoMAP compatibility: replacement Compass movie definition was not provided; preserving fallback behaviour");
							}
						}
					}
					else
					{
						// Some InfinityUI versions may omit the pre-replace event. Bind safely here too.
						CNO::Compass::InitSingleton(newInstance);
						if (auto compass = CNO::Compass::GetSingleton()) {
							compass->SetupMod(newInstance);
							compass->UpdateLayout();
						}
					}
				}
				else if (pathToNew == QuestItemList::path)
				{
					QuestItemList::InitSingleton(newInstance);
					if (auto questItemList = QuestItemList::GetSingleton(); questItemList && questItemList->IsReady()) {
						memberLogger.LogMembersOf(*questItemList);
						if (auto coord = questItemList->LocalToGlobal()) {
							logger::debug("{} is on ({}, {})", questItemList->ToString().c_str(), coord->x, coord->y);
						}
						questItemList->UpdateLayout();
					}
				}
			}
			break;
		case IUI::API::Message::Type::kAbortPatchInstance:
			if (auto abortPatchMessage = IUI::API::TranslateAs<IUI::API::AbortPatchInstanceMessage>(a_msg);
				abortPatchMessage && abortPatchMessage->originalValue.IsDisplayObject())
			{
				std::string pathToOriginal = abortPatchMessage->originalValue.ToString().c_str();
				if (pathToOriginal == CNO::Compass::path) {
					CNO::Compass::InvalidateSingleton();
					hooks::compat::MapMarkerFramework::compassMovieDef = nullptr;
					logger::error("Aborted replacement of {}", CNO::Compass::path);
				} else if (pathToOriginal == QuestItemList::path) {
					QuestItemList::InvalidateSingleton();
					logger::error("Aborted replacement of {}", QuestItemList::path);
				}
			}
			break;
		case IUI::API::Message::Type::kFinishLoadInstances:
			if (auto finishLoadMessage = IUI::API::TranslateAs<IUI::API::FinishLoadInstancesMessage>(a_msg);
				finishLoadMessage && finishLoadMessage->movie)
			{
				RE::GFxValue test;
				if (finishLoadMessage->movie->GetVariable(&test, Test::path.data())) {
					Test::InitSingleton(IUI::GFxDisplayObject{ test, finishLoadMessage->movie });
				}

				// Re-resolve final HUD objects after all replacements. This also recovers safely
				// if a pre/post replacement notification was skipped or a patch was aborted.
				RE::GFxValue finalCompass;
				if (finishLoadMessage->movie->GetVariable(&finalCompass, CNO::Compass::path.data()) && finalCompass.IsDisplayObject()) {
					IUI::GFxDisplayObject finalCompassObject{ finalCompass, finishLoadMessage->movie };
					CNO::Compass::InitSingleton(finalCompassObject);
					if (auto compass = CNO::Compass::GetSingleton(); compass && compass->IsReady()) {
						// Safe recovery path if InfinityUI skipped a pre/post notification.
						compass->SetupMod(finalCompassObject);
					}
				}
				RE::GFxValue finalQuestList;
				if (finishLoadMessage->movie->GetVariable(&finalQuestList, QuestItemList::path.data()) && finalQuestList.IsDisplayObject()) {
					QuestItemList::InitSingleton(IUI::GFxDisplayObject{ finalQuestList, finishLoadMessage->movie });
				}

			}
			RegisterMenuObserver();
			TriggerLayoutRefresh();
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
