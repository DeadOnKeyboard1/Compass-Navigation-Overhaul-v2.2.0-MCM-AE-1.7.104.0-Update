#pragma once

namespace util
{
    inline std::string GetObjectiveDisplayText(const RE::BGSInstancedQuestObjective* a_objective)
    {
        if (!a_objective) {
            return {};
        }

        // CommonLibSSE-NG exposes the current BGSInstancedQuestObjective layout but
        // not this engine helper. Keep the relocation isolated here and immediately
        // copy the BSString into owned storage so callers never retain engine-owned
        // or temporary character buffers.
        RE::BSString result;
        using func_t = void (*)(const RE::BGSInstancedQuestObjective*, RE::BSString&);
        static REL::Relocation<func_t> func{ RELOCATION_ID(23229, 23684) };
        func(a_objective, result);

        const char* text = result.c_str();
        return text ? std::string{ text } : std::string{};
    }

    inline void ReplaceTagsInQuestText(RE::BSString* a_text, const RE::TESQuest* a_quest, std::uint32_t a_questInstanceID)
    {
        if (!a_text || !a_quest) {
            return;
        }

        using func_t = void (*)(RE::BSString*, const RE::TESQuest*, std::uint32_t);
        static REL::Relocation<func_t> func{ RELOCATION_ID(23429, 23897) };
        func(a_text, a_quest, a_questInstanceID);
    }
}
