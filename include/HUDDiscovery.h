#pragma once

#include "IUI/GFxDisplayObject.h"

namespace CNO::HUDDiscovery
{
	struct LocatedDisplayObject
	{
		IUI::GFxDisplayObject object;
		std::string path;
		int score = 0;

		[[nodiscard]] explicit operator bool() const noexcept { return object.IsUsable(); }
	};

	namespace detail
	{
		inline bool IsSkippableMember(std::string_view a_name)
		{
			return a_name == "_parent" || a_name == "__proto__" || a_name == "constructor" ||
				a_name == "_root" || a_name == "_level0";
		}

		inline int ScoreHUDRoot(const RE::GFxValue& a_value)
		{
			if (!a_value.IsObject()) {
				return 0;
			}

			int score = 0;
			RE::GFxValue value;
			if (a_value.GetMember("CompassMarkerList", &value) && value.IsArray()) {
				score += 120;
			}
			if (a_value.GetMember("CompassTargetDataA", &value) && value.IsArray()) {
				score += 120;
			}
			if (a_value.GetMember("HudElements", &value) && value.IsArray()) {
				score += 35;
			}
			if (a_value.HasMember("SetCompassMarkers")) {
				score += 25;
			}
			return score;
		}

		inline int ScoreCompass(const RE::GFxValue& a_value)
		{
			if (!a_value.IsDisplayObject()) {
				return 0;
			}

			int score = 0;
			if (a_value.HasMember("SetFocusedMarkerInfo")) {
				score += 120;
			}
			if (a_value.HasMember("UpdateFocusedMarker")) {
				score += 85;
			}
			if (a_value.HasMember("SetMarkers")) {
				score += 80;
			}
			if (a_value.HasMember("DirectionRect")) {
				score += 55;
			}
			if (a_value.HasMember("CompassMask_mc")) {
				score += 35;
			}
			if (a_value.HasMember("FocusedMarkerInfo")) {
				score += 35;
			}
			if (a_value.HasMember("CompassFrame") || a_value.HasMember("CompassFrameAlt")) {
				score += 15;
			}
			return score;
		}

		inline int ScoreQuestItemList(const RE::GFxValue& a_value)
		{
			if (!a_value.IsDisplayObject()) {
				return 0;
			}

			int score = 0;
			if (a_value.HasMember("AddQuest")) {
				score += 75;
			}
			if (a_value.HasMember("RemoveQuest")) {
				score += 55;
			}
			if (a_value.HasMember("AddToHudElements")) {
				score += 45;
			}
			if (a_value.HasMember("ShowAllQuests")) {
				score += 35;
			}
			if (a_value.HasMember("RemoveAllQuests")) {
				score += 35;
			}
			return score;
		}

		template <class Scorer>
		inline std::optional<LocatedDisplayObject> FindBest(
			RE::GFxMovieView* a_movie,
			Scorer&& a_scorer,
			int a_minimumScore,
			std::span<const std::string_view> a_fastPaths)
		{
			if (!a_movie) {
				return std::nullopt;
			}

			LocatedDisplayObject best;
			for (auto path : a_fastPaths) {
				RE::GFxValue value;
				if (!a_movie->GetVariable(&value, path.data()) || !value.IsDisplayObject()) {
					continue;
				}
				const int score = a_scorer(value);
				if (score > best.score) {
					best = { IUI::GFxDisplayObject{ value, a_movie }, std::string(path), score };
				}
			}

			// Keep the canonical path fast and deterministic when it already identifies
			// a convincing object. Otherwise inspect the active HUD tree without relying
			// on a particular skin's instance names.
			if (best.score >= a_minimumScore + 80) {
				return best;
			}

			struct Node
			{
				RE::GFxValue value;
				std::string path;
				std::uint8_t depth = 0;
			};

			std::deque<Node> queue;
			std::vector<RE::GFxValue> seen;
			seen.reserve(256);

			constexpr std::array<std::string_view, 5> roots{
				"_level0.HUDMovieBaseInstance",
				"_root.HUDMovieBaseInstance",
				"HUDMovieBaseInstance",
				"_level0",
				"_root"
			};

			for (auto rootPath : roots) {
				RE::GFxValue root;
				if (a_movie->GetVariable(&root, rootPath.data()) && root.IsObject()) {
					queue.push_back({ root, std::string(rootPath), 0 });
				}
			}

			auto alreadySeen = [&](const RE::GFxValue& a_value) {
				return std::ranges::any_of(seen, [&](const RE::GFxValue& other) { return other == a_value; });
			};

			constexpr std::size_t kMaxVisited = 384;
			constexpr std::uint8_t kMaxDepth = 7;
			std::size_t visited = 0;

			while (!queue.empty() && visited < kMaxVisited) {
				Node node = std::move(queue.front());
				queue.pop_front();

				if (!node.value.IsObject() || alreadySeen(node.value)) {
					continue;
				}
				seen.push_back(node.value);
				++visited;

				if (node.value.IsDisplayObject()) {
					const int score = a_scorer(node.value);
					if (score > best.score) {
						best = { IUI::GFxDisplayObject{ node.value, a_movie }, node.path, score };
					}
				}

				if (node.depth >= kMaxDepth) {
					continue;
				}

				node.value.VisitMembers([&](const char* a_name, const RE::GFxValue& member) {
					if (!a_name || IsSkippableMember(a_name) || !member.IsDisplayObject() || alreadySeen(member)) {
						return;
					}
					std::string childPath = node.path;
					if (!childPath.empty()) {
						childPath.push_back('.');
					}
					childPath += a_name;
					queue.push_back({ member, std::move(childPath), static_cast<std::uint8_t>(node.depth + 1) });
				});
			}

			if (best.score >= a_minimumScore && best.object.IsUsable()) {
				return best;
			}
			return std::nullopt;
		}
	}

	[[nodiscard]] inline int CompassScore(const RE::GFxValue& a_value)
	{
		return detail::ScoreCompass(a_value);
	}

	[[nodiscard]] inline int QuestItemListScore(const RE::GFxValue& a_value)
	{
		return detail::ScoreQuestItemList(a_value);
	}

	[[nodiscard]] inline bool LooksLikeCompass(const RE::GFxValue& a_value)
	{
		return CompassScore(a_value) >= 70;
	}

	[[nodiscard]] inline bool LooksLikeQuestItemList(const RE::GFxValue& a_value)
	{
		return QuestItemListScore(a_value) >= 120;
	}

	inline std::optional<LocatedDisplayObject> FindCompass(RE::GFxMovieView* a_movie)
	{
		constexpr std::array<std::string_view, 6> paths{
			"_level0.HUDMovieBaseInstance.CompassShoutMeterHolder.Compass",
			"_root.HUDMovieBaseInstance.CompassShoutMeterHolder.Compass",
			"HUDMovieBaseInstance.CompassShoutMeterHolder.Compass",
			"_level0.HUDMovieBaseInstance.Compass",
			"_root.HUDMovieBaseInstance.Compass",
			"HUDMovieBaseInstance.Compass"
		};
		return detail::FindBest(a_movie, detail::ScoreCompass, 70, paths);
	}

	inline std::optional<LocatedDisplayObject> FindQuestItemList(RE::GFxMovieView* a_movie)
	{
		constexpr std::array<std::string_view, 3> paths{
			"_level0.HUDMovieBaseInstance.QuestItemList",
			"_root.HUDMovieBaseInstance.QuestItemList",
			"HUDMovieBaseInstance.QuestItemList"
		};
		return detail::FindBest(a_movie, detail::ScoreQuestItemList, 120, paths);
	}

	inline std::optional<IUI::GFxDisplayObject> FindHUDRoot(RE::GFxMovieView* a_movie, const RE::GFxValue* a_start = nullptr)
	{
		if (!a_movie) {
			return std::nullopt;
		}

		// The closest ancestor that owns Skyrim's marker arrays is the most reliable
		// HUD root and survives simple renaming/reparenting done by UI skins.
		if (a_start && a_start->IsObject()) {
			RE::GFxValue current = *a_start;
			for (std::uint32_t i = 0; i < 10 && current.IsObject(); ++i) {
				if (detail::ScoreHUDRoot(current) >= 200) {
					return IUI::GFxDisplayObject{ current, a_movie };
				}
				RE::GFxValue parent;
				if (!current.GetMember("_parent", &parent) || !parent.IsDisplayObject() || parent == current) {
					break;
				}
				current = std::move(parent);
			}
		}

		constexpr std::array<std::string_view, 3> paths{
			"_level0.HUDMovieBaseInstance",
			"_root.HUDMovieBaseInstance",
			"HUDMovieBaseInstance"
		};
		for (auto path : paths) {
			RE::GFxValue root;
			if (a_movie->GetVariable(&root, path.data()) && root.IsDisplayObject() && detail::ScoreHUDRoot(root) >= 200) {
				return IUI::GFxDisplayObject{ root, a_movie };
			}
		}
		return std::nullopt;
	}

	inline bool IsSameObject(const std::optional<LocatedDisplayObject>& a_located, const RE::GFxValue& a_value)
	{
		return a_located && a_value.IsDisplayObject() && static_cast<const RE::GFxValue&>(a_located->object) == a_value;
	}
}
