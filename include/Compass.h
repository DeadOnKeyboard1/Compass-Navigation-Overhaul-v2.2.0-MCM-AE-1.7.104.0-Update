#pragma once

#include "IUI/GFxArray.h"
#include "IUI/GFxDisplayObject.h"
#include "HUDDiscovery.h"

#include "utils/Geometry.h"

namespace CNO
{
	class Compass : public IUI::GFxDisplayObject
	{
	public:
		struct Marker
		{
			Marker(RE::TESObjectREFR* a_markerRef, float a_angleToPlayerCamera,
				   std::uint32_t a_index, std::uint32_t a_icon, const std::string_view& a_description) :
				ref{ a_markerRef }, angleToPlayerCamera{ a_angleToPlayerCamera },
				index{ a_index }, icon{ a_icon }, description{ a_description }
			{
				if (auto* player = RE::PlayerCharacter::GetSingleton(); player && ref) {
					distanceToPlayer = util::GetDistanceBetween(player, ref);
					heightDifference = util::GetHeightDifferenceBetween(player, ref);
				}
			}

			RE::TESObjectREFR* ref;
			float angleToPlayerCamera;
			float distanceToPlayer = 0.0F;
			float heightDifference = 0.0F;
			std::uint32_t index;
			std::uint32_t icon;
			std::string description;
		};

		static constexpr inline std::string_view path = "_level0.HUDMovieBaseInstance.CompassShoutMeterHolder.Compass";

		static void InitSingleton(const GFxDisplayObject& a_originalCompass, std::string_view a_path = {})
		{
			if (!singleton)
			{
				static Compass singletonInstance{ a_originalCompass };
				singleton = &singletonInstance;
				if (!a_path.empty()) {
					singleton->activePath = a_path;
				}
			}
			else
			{
				// InfinityUI may rebuild only the Compass child while keeping the same
				// CompassShoutMeterHolder alive. Rebind the display object, but do not
				// discard the holder baseline; UpdateLayout stores it on the holder itself.
				*static_cast<GFxDisplayObject*>(singleton) = a_originalCompass;
				if (!a_path.empty()) {
					singleton->activePath = a_path;
				}
			}
		}

		static Compass* GetSingleton() { return singleton; }

		static void InvalidateSingleton()
		{
			if (singleton) {
				singleton->Invalidate();
			}
		}

		[[nodiscard]] bool IsReady() const noexcept { return IsUsable(); }

		[[nodiscard]] bool SupportsCNOFunctions() const
		{
			return IsReady() && HasMember("SetMarkers") && HasMember("SetFocusedMarkerInfo") &&
				HasMember("UpdateFocusedMarker");
		}

		[[nodiscard]] const std::string& GetActivePath() const noexcept { return activePath; }

		void SetupMod(const GFxDisplayObject& a_replaceCompass)
		{
			if (!a_replaceCompass.IsUsable()) {
				return;
			}

			*static_cast<GFxDisplayObject*>(this) = a_replaceCompass;

			if (!SupportsCNOFunctions()) {
				logger::warn("Active compass at '{}' does not expose the CNO ActionScript API; preserving the HUD design and enabling layout-only compatibility",
					activePath.empty() ? "<runtime-discovered>" : activePath);
				return;
			}

			// Initialize the replacement through its own API. Do not assume a fixed
			// HUDMovieBaseInstance/CompassShoutMeterHolder hierarchy.
			if (HasMember("Compass")) {
				Invoke("Compass");
			}

			auto movie = GetMovieView();
			auto hud = HUDDiscovery::FindHUDRoot(movie, this);
			if (hud && hud->IsUsable()) {
				hud->SetMember("__CNO_Compass", *this);
			}
			if (hud && hud->IsUsable() && hud->SetMember("SetCompassMarkers", GetMember("SetMarkers"))) {
				logger::info("Installed CNO compass marker renderer on the runtime-discovered HUD root");
			} else {
				logger::warn("Could not bind CNO SetMarkers to the active HUD root; native marker rendering will be preserved");
			}
		}

		void SetUnits(bool a_useMetric)
		{
			if (HasMember("SetUnits")) { Invoke("SetUnits", a_useMetric); }
		}

		void SetMarkers()
		{
			if (HasMember("SetMarkers")) { Invoke("SetMarkers"); }
		}

		void SetFocusedMarkerInfo(const std::string_view& a_targetText, float a_distance,
								  float a_heightDifference, std::uint32_t a_markerIndex)
		{
			// SetMarkers recreates clips; compatible SWFs may cache the focused clip.
			RE::GFxValue marker, clip;
			auto focused = GetMember("FocusedMarkerInstance");
			auto movie = GetMovieView();
			auto hud = HUDDiscovery::FindHUDRoot(movie, this);
			RE::GFxValue markers;
			if (hud && focused.IsObject() && focused.HasMember("Movie") &&
				static_cast<RE::GFxValue&>(*hud).GetMember("CompassMarkerList", &markers) && markers.IsArray() &&
				a_markerIndex < markers.GetArraySize() && markers.GetElement(a_markerIndex, &marker) &&
				marker.IsObject() && marker.GetMember("movie", &clip))
			{
				focused.SetMember("Movie", clip);
			}
			if (HasMember("SetFocusedMarkerInfo")) {
				const std::string targetText{ a_targetText };
				Invoke("SetFocusedMarkerInfo", targetText.c_str(), a_distance, a_heightDifference, a_markerIndex);
			}
		}

		void FocusMarker(std::uint32_t a_markerIndex)
		{
			if (HasMember("FocusMarker")) { Invoke("FocusMarker", a_markerIndex); }
		}

		void UnfocusMarker()
		{
			if (HasMember("UnfocusMarker")) { Invoke("UnfocusMarker"); }
		}

		void UpdateFocusedMarker()
		{
			if (HasMember("UpdateFocusedMarker")) { Invoke("UpdateFocusedMarker"); }
		}

		void PostProcessMarkers(const std::unordered_map<std::uint32_t, bool>& a_unknownLocations, std::uint32_t a_markersCount)
		{
			auto* movieView = GetMovieView();
			if (!IsReady() || !movieView) {
				return;
			}
			GFxArray gfxIsUnknownLocations{ movieView };
			if (!gfxIsUnknownLocations.IsUsable()) {
				return;
			}

			for (std::uint32_t i = 0; i < a_markersCount; ++i) {
				gfxIsUnknownLocations.PushBack(a_unknownLocations.contains(i));
			}

			if (HasMember("PostProcessMarkers")) {
				Invoke("PostProcessMarkers", gfxIsUnknownLocations);
			}
		}

		void UpdateLayout()
		{
			auto movieView = GetMovieView();
			if (!movieView || !IsReady())
			{
				return;
			}

			// The immediate parent is the authoritative layout holder for the active
			// compass skin. This deliberately avoids assuming the vanilla
			// CompassShoutMeterHolder name/path.
			RE::GFxValue holder = GetMember("_parent");
			const bool foundHolder = holder.IsDisplayObject();

			if (foundHolder)
			{
				if (auto hud = HUDDiscovery::FindHUDRoot(movieView, this); hud && hud->IsUsable()) {
					hud->SetMember("__CNO_CompassHolder", holder);
				}
				// Store the unmodified holder transform on the ActionScript object itself.
				// Fast travel can replace only the Compass child while preserving this holder.
				// Reading the current holder transform again in that case would capture CNO's
				// already-applied offset/scale and apply the MCM values a second time.
				constexpr std::string_view kBaseX = "__CNO_BaseX";
				constexpr std::string_view kBaseY = "__CNO_BaseY";
				constexpr std::string_view kBaseScaleX = "__CNO_BaseScaleX";
				constexpr std::string_view kBaseScaleY = "__CNO_BaseScaleY";

				RE::GFxValue baseXVal, baseYVal, baseScaleXVal, baseScaleYVal;
				bool hasStoredBaseline =
					holder.GetMember(kBaseX.data(), &baseXVal) && baseXVal.IsNumber() &&
					holder.GetMember(kBaseY.data(), &baseYVal) && baseYVal.IsNumber() &&
					holder.GetMember(kBaseScaleX.data(), &baseScaleXVal) && baseScaleXVal.IsNumber() &&
					holder.GetMember(kBaseScaleY.data(), &baseScaleYVal) && baseScaleYVal.IsNumber();

				if (hasStoredBaseline) {
					const double rawBaseX = baseXVal.GetNumber();
					const double rawBaseY = baseYVal.GetNumber();
					const double rawBaseScaleX = baseScaleXVal.GetNumber();
					const double rawBaseScaleY = baseScaleYVal.GetNumber();
					hasStoredBaseline = std::isfinite(rawBaseX) && std::isfinite(rawBaseY) &&
						std::isfinite(rawBaseScaleX) && rawBaseScaleX > 0.0 &&
						std::isfinite(rawBaseScaleY) && rawBaseScaleY > 0.0;
				}

				float baseHolderX = 0.0F;
				float baseHolderY = 0.0F;
				float baseHolderScaleX = 100.0F;
				float baseHolderScaleY = 100.0F;

				if (hasStoredBaseline)
				{
					baseHolderX = static_cast<float>(baseXVal.GetNumber());
					baseHolderY = static_cast<float>(baseYVal.GetNumber());
					baseHolderScaleX = static_cast<float>(baseScaleXVal.GetNumber());
					baseHolderScaleY = static_cast<float>(baseScaleYVal.GetNumber());
				}
				else
				{
					RE::GFxValue xVal, yVal, sxVal, syVal;
					holder.GetMember("_x", &xVal);
					holder.GetMember("_y", &yVal);
					holder.GetMember("_xscale", &sxVal);
					holder.GetMember("_yscale", &syVal);

					const double rawX = xVal.IsNumber() ? xVal.GetNumber() : 0.0;
					const double rawY = yVal.IsNumber() ? yVal.GetNumber() : 0.0;
					const double rawScaleX = sxVal.IsNumber() ? sxVal.GetNumber() : 100.0;
					const double rawScaleY = syVal.IsNumber() ? syVal.GetNumber() : 100.0;

					baseHolderX = std::isfinite(rawX) ? static_cast<float>(rawX) : 0.0F;
					baseHolderY = std::isfinite(rawY) ? static_cast<float>(rawY) : 0.0F;
					baseHolderScaleX = (std::isfinite(rawScaleX) && rawScaleX > 0.0) ? static_cast<float>(rawScaleX) : 100.0F;
					baseHolderScaleY = (std::isfinite(rawScaleY) && rawScaleY > 0.0) ? static_cast<float>(rawScaleY) : 100.0F;

					holder.SetMember(kBaseX.data(), baseHolderX);
					holder.SetMember(kBaseY.data(), baseHolderY);
					holder.SetMember(kBaseScaleX.data(), baseHolderScaleX);
					holder.SetMember(kBaseScaleY.data(), baseHolderScaleY);

					logger::info("Captured native CompassHolder baseline: pos({:.1f},{:.1f}) scale=({:.1f}%, {:.1f}%)",
						baseHolderX, baseHolderY, baseHolderScaleX, baseHolderScaleY);
				}

				const float scaleMultiplier = std::max(settings::compass::scale, 1.0F) / 100.0F;
				const float newX = baseHolderX + settings::compass::offsetX;
				const float newY = baseHolderY + settings::compass::offsetY;
				const float newScaleX = baseHolderScaleX * scaleMultiplier;
				const float newScaleY = baseHolderScaleY * scaleMultiplier;

				if (!std::isfinite(newX) || !std::isfinite(newY) || !std::isfinite(newScaleX) || !std::isfinite(newScaleY)) {
					logger::warn("Compass UpdateLayout produced a non-finite transform; leaving the active HUD transform untouched");
					return;
				}

				// Always write the calculated values. This is required to restore the captured
				// native UI layout after the user resets the MCM values to 0/0/100.
				holder.SetMember("_x", newX);
				holder.SetMember("_y", newY);
				holder.SetMember("_xscale", newScaleX);
				holder.SetMember("_yscale", newScaleY);

				logger::info("Compass UpdateLayout: holder pos({:.1f},{:.1f}) scale=({:.1f}%, {:.1f}%) [base=({:.1f},{:.1f}), baseScale=({:.1f}%, {:.1f}%), mult={:.1f}%]",
					newX, newY, newScaleX, newScaleY, baseHolderX, baseHolderY, baseHolderScaleX, baseHolderScaleY, settings::compass::scale);
			}
			else
			{
				logger::warn("Compass UpdateLayout: runtime compass parent/holder not found; leaving the active HUD transform untouched");
			}

			SetUnits(settings::display::useMetricUnits);
		}

	private:
		Compass(const GFxDisplayObject& a_originalCompass) : GFxDisplayObject{ a_originalCompass }
		{}

		static inline Compass* singleton = nullptr;
		std::string activePath{ path };
	};
}
