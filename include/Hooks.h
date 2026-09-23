#pragma once

#include "utils/Trampoline.h"

#include "RE/B/BSCoreTypes.h"
#include "RE/C/Compass.h"
#include "RE/H/HUDMarkerManager.h"
#include "RE/N/NiPoint3.h"

namespace hooks
{
	bool UpdateQuests(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
					  RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame);

	RE::TESWorldSpace* AllowedToShowMapMarker(const RE::TESObjectREFR* a_marker);

	bool UpdateLocations(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
						 RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame);

	bool UpdateEnemies(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
					   RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame);

	bool UpdatePlayerSetMarker(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
							   RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame);

	void UpdateCompass(RE::Compass* a_compass);

	class HUDMarkerManager
	{
		static constexpr REL::RelocationID UpdateQuestsId{ 50826, 51691 };
		static constexpr REL::RelocationID UpdateLocationsId{ 50870, 51744 };
		static constexpr REL::RelocationID AddMarkerId{ 50851, 51728 };

	public:

		// These two relocations are used only as function-start addresses. Keeping
		// invented function signatures here would reintroduce ABI assumptions.
		static inline REL::Relocation<std::uintptr_t> UpdateQuests{ UpdateQuestsId };

		static inline REL::Relocation<std::uintptr_t> UpdateLocations{ UpdateLocationsId };

		static inline REL::Relocation<bool(*)(const RE::HUDMarkerManager*, RE::HUDMarker::ScaleformData*,
											  RE::NiPoint3*, const RE::RefHandle&, std::int32_t)> AddMarker{ AddMarkerId };
	};

	class HUDMenu
	{
		static constexpr REL::RelocationID ProcessMessageId{ 50718, 51612 };

	public:

		static inline REL::Relocation<std::uintptr_t> ProcessMessage{ ProcessMessageId };
	};

	class Compass
	{
		static constexpr REL::RelocationID SetMarkersId{ 50775, 51670 };
		static constexpr REL::RelocationID UpdateId{ 50773, 51668 };

	public:
		static inline REL::Relocation<std::uintptr_t> vTable{ RE::VTABLE_Compass[0] };

		static inline REL::Relocation<bool (*)(RE::Compass*)> SetMarkers{ SetMarkersId };
		static inline REL::Relocation<void (*)(RE::Compass*)> Update{ UpdateId };
	};

	static inline bool Install()
	{
		// `HUDMarkerManager::UpdateQuests` (call to `HUDMarkerManager::AddMarker`).
		// Hook the call directly. Do not reinterpret internal loop registers as quest structures.
		struct UpdateQuestsHook : Hook<5>
		{
			static std::uintptr_t Address() { return HUDMarkerManager::UpdateQuests.address() + REL::VariantOffset{ 0x114, 0x180, 0x114 }.offset(); }

			UpdateQuestsHook(std::uintptr_t a_hookedAddress) :
				Hook{ a_hookedAddress, reinterpret_cast<std::uintptr_t>(&UpdateQuests) }
			{}
		};

		// `HUDMarkerManager::UpdateLocations` (calls to `TESObjectREFR::GetWorldspace`)
		struct AllowedToShowMapMarkerHook : Hook<5>
		{
			static std::uintptr_t Address1() { return HUDMarkerManager::UpdateLocations.address() + REL::VariantOffset{ 0x139, 0x13C, 0x139 }.offset(); }
			static std::uintptr_t Address2() { return HUDMarkerManager::UpdateLocations.address() + REL::VariantOffset{ 0x21C, 0x24B, 0x21C }.offset(); }

			AllowedToShowMapMarkerHook(std::uintptr_t a_hookedAddress) :
				Hook{ a_hookedAddress, reinterpret_cast<std::uintptr_t>(&AllowedToShowMapMarker) }
			{}
		};

		// `HUDMarkerManager::UpdateLocations` (call to `HUDMarkerManager::AddMarker` for locations)
		struct UpdateLocationsHook : Hook<5>
		{
			static std::uintptr_t Address() { return HUDMarkerManager::UpdateLocations.address() + REL::VariantOffset{ 0x450, 0x473, 0x450 }.offset(); }

			UpdateLocationsHook(std::uintptr_t a_hookedAddress) :
				Hook{ a_hookedAddress, reinterpret_cast<std::uintptr_t>(&UpdateLocations) }
			{}
		};

		// `HUDMenu::ProcessMessage` (call to `HUDMarkerManager::AddMarker` for enemies)
		struct UpdateEnemiesHook : Hook<5>
		{
			// Offset changed from 1.6.640 -> 1.6.1130, so we gotta be more granular with the versions
			static std::uintptr_t Address()
			{
				REL::Version version = REL::Module::get().version();
				std::uintptr_t offsetAE = version < REL::Version{ 1, 6, 1130, 0 } ? 0x1695 : 0x1735;

				return HUDMenu::ProcessMessage.address() + REL::VariantOffset{ 0x15AB, offsetAE, 0x15AB }.offset();
			}

			UpdateEnemiesHook(std::uintptr_t a_hookedAddress) :
				Hook{ a_hookedAddress, reinterpret_cast<std::uintptr_t>(&UpdateEnemies) }
			{}
		};

		// `Compass::SetMarkers` (call to `HUDMarkerManager::AddMarker` for Player-set marker)
		struct UpdatePlayerSetMarkerHook : Hook<5>
		{
			// In AE `Compass::SetMarkers` is inlined in `Compass::Update`
			static std::uintptr_t Address() { return REL::Module::IsAE() ? Compass::Update.address() + 0xAE : Compass::SetMarkers.address() + 0x8D; }

			UpdatePlayerSetMarkerHook(std::uintptr_t a_hookedAddress) :
				Hook{ a_hookedAddress, reinterpret_cast<std::uintptr_t>(&UpdatePlayerSetMarker) }
			{}
		};

		const auto updateQuestsAddress = UpdateQuestsHook::Address();
		const auto allowedMarkerAddress1 = AllowedToShowMapMarkerHook::Address1();
		const auto allowedMarkerAddress2 = AllowedToShowMapMarkerHook::Address2();
		const auto updateLocationsAddress = UpdateLocationsHook::Address();
		const auto updateEnemiesAddress = UpdateEnemiesHook::Address();
		const auto updatePlayerSetMarkerAddress = UpdatePlayerSetMarkerHook::Address();

		auto relativeCallTarget = [](std::uintptr_t a_address) -> std::uintptr_t {
			if (!a_address) {
				return 0;
			}
			MEMORY_BASIC_INFORMATION mbi{};
			if (!VirtualQuery(reinterpret_cast<const void*>(a_address), &mbi, sizeof(mbi)) ||
				mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 ||
				reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize < a_address + 5) {
				return 0;
			}
			if (*reinterpret_cast<const std::uint8_t*>(a_address) != 0xE8) {
				return 0;
			}

			std::int32_t displacement = 0;
			std::memcpy(&displacement, reinterpret_cast<const void*>(a_address + 1), sizeof(displacement));
			return a_address + 5 + displacement;
		};

		const auto addMarkerAddress = HUDMarkerManager::AddMarker.address();
		const auto allowedTarget1 = relativeCallTarget(allowedMarkerAddress1);
		const auto allowedTarget2 = relativeCallTarget(allowedMarkerAddress2);
		if (relativeCallTarget(updateQuestsAddress) != addMarkerAddress ||
			relativeCallTarget(updateLocationsAddress) != addMarkerAddress ||
			relativeCallTarget(updateEnemiesAddress) != addMarkerAddress ||
			relativeCallTarget(updatePlayerSetMarkerAddress) != addMarkerAddress ||
			!allowedTarget1 || allowedTarget1 != allowedTarget2)
		{
			SKSE::log::critical("CNO hook validation failed: Skyrim 1.7.104.0 patch sites no longer target the expected functions; refusing to patch unknown code");
			return false;
		}

		UpdateQuestsHook updateQuestsHook{ updateQuestsAddress };
		AllowedToShowMapMarkerHook allowedToShowMapMarkerHook[2]{ allowedMarkerAddress1, allowedMarkerAddress2 };
		UpdateLocationsHook updateLocationsHook{ updateLocationsAddress };
		UpdateEnemiesHook updateEnemiesHook{ updateEnemiesAddress };
		UpdatePlayerSetMarkerHook updatePlayerSetMarkerHook{ updatePlayerSetMarkerAddress };
		
		// The destination of the hook for `AllowedToShowMapMarker` is the same,
		// so we need to allocate memory for it only once
		static DefaultTrampoline defaultTrampoline{ updateQuestsHook.getSize() + allowedToShowMapMarkerHook->getSize() +
													updateLocationsHook.getSize() + updateEnemiesHook.getSize() +
													updatePlayerSetMarkerHook.getSize() };
		
		defaultTrampoline.write_call(updateQuestsHook);
		defaultTrampoline.write_call(allowedToShowMapMarkerHook[0]);
		defaultTrampoline.write_call(allowedToShowMapMarkerHook[1]);
		defaultTrampoline.write_call(updateLocationsHook);
		defaultTrampoline.write_call(updateEnemiesHook);
		defaultTrampoline.write_call(updatePlayerSetMarkerHook);

		Compass::vTable.write_vfunc(1, UpdateCompass);
		return true;
	}

	namespace compat
	{
		namespace AlternatePerspective
		{
			void OnDataLoaded();
		}

		class MapMarkerFramework
		{
		public:

			static RE::GFxMovieDef* GetCompassMovieDef(void* a_originalRCX, RE::GFxMovieView* a_movieView);

			static inline bool Install(REX::W32::HMODULE a_moduleHandle)
			{
				// `ImportManager::SetupHUDMenu` call to a_movieView->GetMovieDef()
				struct GetCompassMovieDefHook : Hook<6>
				{
					GetCompassMovieDefHook(std::uintptr_t a_hookedAddress) :
						Hook{ a_hookedAddress, reinterpret_cast<std::uintptr_t>(&GetCompassMovieDef) }
					{}
				};

				if (!a_moduleHandle) {
					SKSE::log::warn("CoMAP compatibility: MapMarkerFramework.dll module handle is null; skipping patch");
					return false;
				}

				const std::uintptr_t patternAddress = SigScanner::FindPattern
				<
					"4C 8B F1 "	// mov     r14, rcx
					"48 8B 02 "	// mov     rax, [rdx]
					"48 8B CA "	// mov     rcx, rdx <- We want this address (offset 6)
					"FF 50 08 "	// call    qword ptr [rax+8]
					"?? 8B ?? "	// mov     ??, rax
					"48 85 C0"	// test    rax, rax
				>(a_moduleHandle);

				if (!patternAddress) {
					SKSE::log::warn("CoMAP compatibility: expected MapMarkerFramework signature was not found; skipping patch instead of installing an invalid hook");
					return false;
				}

				const std::uintptr_t getCompassMovieDefHookAddress = patternAddress + 6;
				GetCompassMovieDefHook getCompassMovieDefHook{ getCompassMovieDefHookAddress };

				static CustomTrampoline mapMarkerFrameworkTrampoline{ "MapMarkerFramework Trampoline", a_moduleHandle,
																	  getCompassMovieDefHook.getSize() };

				if (!mapMarkerFrameworkTrampoline.IsValid()) {
					SKSE::log::warn("CoMAP compatibility: failed to allocate trampoline memory; skipping patch");
					return false;
				}

				mapMarkerFrameworkTrampoline.write_call(getCompassMovieDefHook);
				return true;
			}

			static inline const SKSE::PluginInfo* pluginInfo = nullptr;
			static inline RE::GFxMovieDef* compassMovieDef = nullptr;
		};
	}
}
