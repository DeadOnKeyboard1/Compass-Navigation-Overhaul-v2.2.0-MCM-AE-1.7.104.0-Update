#pragma once

#include <numbers>

namespace util
{
	static constexpr float pi = std::numbers::pi_v<float>;

	constexpr void CropAngleRange(float& a_angle)
	{
		if (a_angle <= 2 * pi)
		{
			if (a_angle < 0.0F)
			{
				a_angle = std::fmodf(a_angle, 2 * pi) + 2 * pi;
			}
		}
		else
		{
			a_angle = std::fmodf(a_angle, 2 * pi);
		}
	};

	constexpr float RadiansToDegrees(float a_angle)
	{
		return a_angle * 180.0F / pi;
	}

	inline float GetAngleBetween(const RE::PlayerCamera* a_playerCamera, const RE::TESObjectREFR* a_markerRef)
	{
		if (!a_playerCamera || !a_markerRef) {
			return 0.0F;
		}

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return 0.0F;
		}

		const RE::NiPoint3 playerPos = player->GetPosition();
		const RE::NiPoint3 markerPos = a_markerRef->GetPosition();

		// Skyrim positions compass markers against the player/camera angle.
		// Interior north rotation affects the compass/cardinal strip, but it is not
		// part of the marker-centering angle used by HUDMenu::UpdateCompassMarkers.
		// Adding GetNorthRotation() here shifts CNO's invisible focus test away from
		// the marker that is actually drawn on screen in rotated interior cells.
		float playerCameraAngle = a_playerCamera->yaw;

		const float diffX = markerPos.x - playerPos.x;
		const float diffY = markerPos.y - playerPos.y;
		float headingAngle = std::atan2(diffX, diffY);

		CropAngleRange(playerCameraAngle);
		CropAngleRange(headingAngle);

		float angle = headingAngle - playerCameraAngle;
		CropAngleRange(angle);
		return angle;
	}

	inline RE::NiPoint3 GetRealPosition(const RE::TESObjectREFR* a_objRef)
	{
		if (!a_objRef) {
			return RE::NiPoint3::Zero();
		}

		RE::NiPoint3 position = a_objRef->GetPosition();

		if (const RE::TESWorldSpace* worldSpace = a_objRef->GetWorldspace())
		{
			RE::NiPoint3 worldSpaceOffset{ worldSpace->worldMapOffsetData.mapOffsetX,
												  worldSpace->worldMapOffsetData.mapOffsetY,
												  worldSpace->worldMapOffsetData.mapOffsetZ };

			position += worldSpaceOffset * worldSpace->worldMapOffsetData.mapScale;
		}

		return position;
	}

	inline float GetDistanceBetween(const RE::PlayerCharacter* a_player, const RE::TESObjectREFR* a_marker)
	{
		if (!a_player || !a_marker) {
			return 0.0F;
		}

		RE::NiPoint3 playerPos = GetRealPosition(a_player);
		RE::NiPoint3 markerPos = GetRealPosition(a_marker);

		return playerPos.GetDistance(markerPos);
	}

	inline float GetHeightDifferenceBetween(const RE::PlayerCharacter* a_player, const RE::TESObjectREFR* a_marker)
	{
		if (!a_player || !a_marker) {
			return 0.0F;
		}

		RE::NiPoint3 playerPos = GetRealPosition(a_player);
		RE::NiPoint3 markerPos = GetRealPosition(a_marker);

		return markerPos.z - playerPos.z;
	}
}