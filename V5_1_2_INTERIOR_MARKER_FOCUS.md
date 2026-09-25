# v5.1.2 - Interior marker focus alignment

## Symptom

In some interior cells, CNO could show quest details while the visible quest marker was not centered, or fail to show them while the marker was centered.

## Cause

CNO's focus-angle helper added `TESObjectCELL::GetNorthRotation()` to the camera yaw. Skyrim's HUD separates the compass/cardinal-strip angle from marker centering: `HUDMenu::SetCompassAngle` passes the player angle to `UpdateCompassMarkers`, and that player angle is what the marker headings are compared against. Adding the cell north rotation to CNO's focus test therefore rotated the invisible focus axis away from the visible marker in interiors with a non-zero north rotation.

## Fix

`util::GetAngleBetween` now compares marker heading against the player/camera yaw only. This restores the original CNO centering semantics and matches Skyrim's marker-placement logic.

## Scope

Only the focus-angle calculation changed. Marker hooks, marker indices, quest aggregation, MCM settings, universal HUD discovery, fast-travel layout handling, ESP, translations and SWF packaging are unchanged.
