#pragma once

#include "IUI/GFxArray.h"
#include "IUI/GFxDisplayObject.h"

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
			{}

			RE::TESObjectREFR* ref;
			float angleToPlayerCamera;
			float distanceToPlayer = util::GetDistanceBetween(RE::PlayerCharacter::GetSingleton(), ref);
			float heightDifference = util::GetHeightDifferenceBetween(RE::PlayerCharacter::GetSingleton(), ref);
			std::uint32_t index;
			std::uint32_t icon;
			std::string description;
		};

		static constexpr inline std::string_view path = "_level0.HUDMovieBaseInstance.CompassShoutMeterHolder.Compass";

		static void InitSingleton(const GFxDisplayObject& a_originalCompass)
		{
			if (!singleton)
			{
				static Compass singletonInstance{ a_originalCompass };
				singleton = &singletonInstance;
			}
		}

		static Compass* GetSingleton() { return singleton; }

		void SetupMod(const GFxDisplayObject& a_replaceCompass)
		{
			if (a_replaceCompass.HasMember("Compass"))
			{
				*static_cast<GFxDisplayObject*>(this) = a_replaceCompass;

				Invoke("Compass");

				// Some compatible SWFs expose SetMarkers without installing the HUD callback.
				RE::GFxValue hud;
				auto movie = GetMovieView();
				if (HasMember("SetMarkers") && movie &&
					movie->GetVariable(&hud, "_level0.HUDMovieBaseInstance") && hud.IsObject() &&
					hud.SetMember("SetCompassMarkers", GetMember("SetMarkers")))
				{
					logger::info("Installed CNO compass marker renderer (unknown-location symbols enabled)");
				}
				else
				{
					logger::error("Could not install CNO compass marker renderer; check the active Compass.swf");
				}
			}
		}

		void SetUnits(bool a_useMetric)
		{
			Invoke("SetUnits", a_useMetric);
		}

		void SetMarkers()
		{
			Invoke("SetMarkers");
		}

		void SetFocusedMarkerInfo(const std::string_view& a_targetText, float a_distance,
								  float a_heightDifference, std::uint32_t a_markerIndex)
		{
			// SetMarkers recreates clips; compatible SWFs may cache the focused clip.
			RE::GFxValue markers, marker, clip;
			auto focused = GetMember("FocusedMarkerInstance");
			auto movie = GetMovieView();
			if (focused.IsObject() && focused.HasMember("Movie") && movie &&
				movie->GetVariable(&markers, "_level0.HUDMovieBaseInstance.CompassMarkerList") &&
				markers.IsArray() && a_markerIndex < markers.GetArraySize() &&
				markers.GetElement(a_markerIndex, &marker) && marker.IsObject() &&
				marker.GetMember("movie", &clip))
			{
				focused.SetMember("Movie", clip);
			}
			Invoke("SetFocusedMarkerInfo", a_targetText.data(), a_distance, a_heightDifference,
										   a_markerIndex);
		}

		void FocusMarker(std::uint32_t a_markerIndex)
		{
			Invoke("FocusMarker", a_markerIndex);
		}

		void UnfocusMarker()
		{
			Invoke("UnfocusMarker");
		}

		void UpdateFocusedMarker()
		{
			Invoke("UpdateFocusedMarker");
		}

		void PostProcessMarkers(const std::unordered_map<std::uint32_t, bool>& a_unknownLocations, std::uint32_t a_markersCount)
		{
			GFxArray gfxIsUnknownLocations{ GetMovieView() };

			for (std::uint32_t i = 0; i < a_markersCount; i++)
			{
				gfxIsUnknownLocations.PushBack(a_unknownLocations.contains(i));
			}

			Invoke("PostProcessMarkers", gfxIsUnknownLocations);
		}

		void UpdateLayout()
		{
			auto movieView = GetMovieView();
			if (!movieView || !IsObject())
			{
				return;
			}

			RE::GFxValue holder;
			bool foundHolder = false;
			if (movieView->GetVariable(&holder, "_level0.HUDMovieBaseInstance.CompassShoutMeterHolder") && holder.IsObject())
			{
				foundHolder = true;
			}
			else
			{
				holder = GetMember("_parent");
				if (holder.IsObject())
				{
					foundHolder = true;
				}
			}

			if (foundHolder)
			{
				if (baseHolderX < -99990.0F)
				{
					RE::GFxValue xVal, yVal;
					holder.GetMember("_x", &xVal);
					holder.GetMember("_y", &yVal);
					baseHolderX = xVal.IsNumber() ? static_cast<float>(xVal.GetNumber()) : 0.0F;
					baseHolderY = yVal.IsNumber() ? static_cast<float>(yVal.GetNumber()) : 0.0F;
					logger::info("Captured base CompassHolder position: ({:.1f}, {:.1f})", baseHolderX, baseHolderY);
				}

				if (baseHolderScaleX < -99990.0F)
				{
					RE::GFxValue sxVal, syVal;
					holder.GetMember("_xscale", &sxVal);
					holder.GetMember("_yscale", &syVal);
					baseHolderScaleX = (sxVal.IsNumber() && sxVal.GetNumber() > 0) ? static_cast<float>(sxVal.GetNumber()) : 100.0F;
					baseHolderScaleY = (syVal.IsNumber() && syVal.GetNumber() > 0) ? static_cast<float>(syVal.GetNumber()) : 100.0F;
					logger::info("Captured base CompassHolder scale: ({:.1f}%, {:.1f}%)", baseHolderScaleX, baseHolderScaleY);
				}

				bool hasCustomCompass = (settings::compass::offsetX != 0.0F || settings::compass::offsetY != 0.0F || std::abs(settings::compass::scale - 100.0F) > 0.01F);
				if (hasCustomCompass)
				{
					float newX = baseHolderX + settings::compass::offsetX;
					float newY = baseHolderY + settings::compass::offsetY;
					float newScaleX = baseHolderScaleX * (settings::compass::scale / 100.0F);
					float newScaleY = baseHolderScaleY * (settings::compass::scale / 100.0F);

					holder.SetMember("_x", newX);
					holder.SetMember("_y", newY);
					holder.SetMember("_xscale", newScaleX);
					holder.SetMember("_yscale", newScaleY);

					logger::info("Compass UpdateLayout: holder pos({:.1f},{:.1f}) scale=({:.1f}%, {:.1f}%) [base={:.1f}%, mult={:.1f}%]",
						newX, newY, newScaleX, newScaleY, baseHolderScaleX, settings::compass::scale);
				}
				else
				{
					logger::info("Compass UpdateLayout: UI Mod native layout preserved (pos {:.1f},{:.1f}, scale {:.1f}%)",
						baseHolderX, baseHolderY, baseHolderScaleX);
				}
			}
			else
			{
				logger::warn("Compass UpdateLayout: CompassShoutMeterHolder not found");
			}

			SetUnits(settings::display::useMetricUnits);
		}

	private:
		Compass(const GFxDisplayObject& a_originalCompass) : GFxDisplayObject{ a_originalCompass }
		{}

		static inline Compass* singleton = nullptr;
		static inline float baseHolderX = -99999.0F;
		static inline float baseHolderY = -99999.0F;
		static inline float baseHolderScaleX = -99999.0F;
		static inline float baseHolderScaleY = -99999.0F;
	};
}
