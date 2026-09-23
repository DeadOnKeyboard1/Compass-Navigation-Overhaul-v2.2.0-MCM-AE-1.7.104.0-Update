#include "NND/NPCNameProvider.h"

namespace logger = SKSE::log;

namespace NND
{
	std::string NPCNameProvider::GetName(RE::Actor* actor) const
	{
		if (!actor) {
			return {};
		}

		if (nnd) {
			if (auto name = nnd->GetName(actor, API::NameContext::kEnemyHUD); !name.empty()) {
				return std::string{ name };
			}
		}

		const char* fallback = actor->GetDisplayFullName();
		return fallback ? std::string{ fallback } : std::string{};
	}

	void NPCNameProvider::RequestAPI()
	{
		if (!nnd) {
			nnd = static_cast<API::IVNND1*>(API::RequestPluginAPI(API::InterfaceVersion::kV1));
			if (nnd) {
				logger::info("Obtained NND API - {0:x}", reinterpret_cast<uintptr_t>(nnd));
			} else {
				logger::warn("Failed to obtain NND API");
			}
		}
	}
}
