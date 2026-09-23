#pragma once

#include "GFxObject.h"

namespace IUI
{
	class GFxDisplayObject : public GFxObject
	{
	public:
		GFxDisplayObject() = default;

		GFxDisplayObject(const RE::GFxValue& a_value, RE::GFxMovieView* a_movieView) :
			GFxObject{ a_value, a_movieView }
		{}

		GFxDisplayObject CreateEmptyMovieClip(const std::string_view& a_name, std::int32_t a_depth)
		{
			RE::GFxValue mc;
			if (!IsUsable() || !RE::GFxValue::CreateEmptyMovieClip(&mc, a_name.data(), a_depth) || !mc.IsDisplayObject()) {
				return {};
			}
			return GFxDisplayObject{ mc, GetMovieView() };
		}

		std::int32_t GetNextHighestDepth()
		{
			auto result = Invoke("getNextHighestDepth");
			return result.IsNumber() ? static_cast<std::int32_t>(result.GetNumber()) : 0;
		}

		void SwapDepths(std::int32_t a_depth) { Invoke("swapDepths", a_depth); }
		void LoadMovie(const std::string_view& a_swfRelativePath) { Invoke("loadMovie", a_swfRelativePath.data()); }
		void RemoveMovieClip() { Invoke("removeMovieClip"); }

		std::optional<RE::GPointF> LocalToGlobal()
		{
			auto* movieView = GetMovieView();
			if (!movieView || !IsUsable()) {
				return std::nullopt;
			}

			GFxObject pt(movieView);
			if (!pt.IsUsable()) {
				return std::nullopt;
			}
			pt.SetMember("x", 0.0);
			pt.SetMember("y", 0.0);
			RE::GFxValue arg = pt;
			if (!RE::GFxValue::Invoke("localToGlobal", nullptr, &arg, 1)) {
				return std::nullopt;
			}

			auto x = pt.GetMember("x");
			auto y = pt.GetMember("y");
			if (!x.IsNumber() || !y.IsNumber()) {
				return std::nullopt;
			}
			return RE::GPointF{ static_cast<float>(x.GetNumber()), static_cast<float>(y.GetNumber()) };
		}
	};
}
