#pragma once

#include "RE/E/ExtraMapMarker.h"
#include "RE/G/GFxValue.h"

namespace RE
{
	class HUDMarker
	{
	public:
		struct FrameOffsets
		{
			static FrameOffsets* GetSingleton()
			{
				REL::Relocation<FrameOffsets*> singleton{ RELOCATION_ID(519587, 406118) };
				return singleton.get();
			}

			std::uint32_t quest;
			std::uint32_t questDoor;
			std::uint32_t playerSet;
			std::uint32_t enemy;
			std::uint32_t location;
			std::uint32_t undiscoveredLocation;
		};
		static_assert(sizeof(FrameOffsets) == 0x18);

		struct ScaleformData
		{
			GFxValue heading;
			GFxValue alpha;
			GFxValue icon;
			GFxValue scale;
		};
		static_assert(sizeof(ScaleformData) == 0x60);

		bool IsQuestFrame(std::uint32_t a_frame) const
		{
			auto* frameOffsets = FrameOffsets::GetSingleton();
			return frameOffsets && (a_frame == frameOffsets->quest || a_frame == frameOffsets->questDoor);
		}
	};

	// Verified layout used by the original CNO 1.7.104 port.  Keep access to the
	// marker buffer narrowly bounded: CNO only reads currentMarkerIndex/radius and
	// writes the icon of the just-added marker.
	class HUDMarkerManager
	{
	public:
		static HUDMarkerManager* GetSingleton()
		{
			REL::Relocation<HUDMarkerManager*> singleton{ RELOCATION_ID(519611, 406154) };
			return singleton.get();
		}

		[[nodiscard]] std::optional<std::uint32_t> GetLastMarkerIndex() const noexcept
		{
			constexpr std::uint32_t kMarkerCapacity = 49;
			if (currentMarkerIndex == 0 || currentMarkerIndex > kMarkerCapacity) {
				return std::nullopt;
			}
			return currentMarkerIndex - 1;
		}

		HUDMarker::ScaleformData scaleformMarkerData[49];  // 0000
		NiPoint3 position[48];                             // 1260
		BSTArray<RefHandle> locationRefs;                  // 14A0
		float sqRadiusToAddLocation;                       // 14B8
		std::uint32_t currentMarkerIndex;                  // 14BC
		std::uint32_t unk14C0;                             // 14C0
	};
	static_assert(sizeof(HUDMarkerManager) == 0x14C8);
}
