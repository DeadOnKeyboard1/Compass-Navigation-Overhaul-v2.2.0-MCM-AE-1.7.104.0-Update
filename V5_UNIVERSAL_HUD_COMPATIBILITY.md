# v5 Universal HUD Compatibility

Target: Skyrim AE 1.7.104.0

## Goal

Keep CNO functionality independent from the visual compass skin wherever the active HUD still exposes Skyrim/CNO-compatible compass semantics.

## Runtime strategy

1. Try the known CNO/Vanilla paths as fast fallbacks.
2. If those are not a convincing match, search the active HUD Scaleform tree with strict depth/visit limits.
3. Score candidate display objects by capabilities such as `SetMarkers`, `SetFocusedMarkerInfo`, `UpdateFocusedMarker`, `DirectionRect`, `CompassMask_mc`, `AddQuest`, and `AddToHudElements`.
4. Bind the actual live object and use its immediate parent as the layout holder.
5. Capture that holder's native transform once and treat it as MCM 0/0/100.
6. If CNO ActionScript functions are missing, leave the HUD's native marker renderer alone and use only safe layout compatibility.
7. Treat the quest-list overlay as optional so a UI replacer cannot disable the entire compass path just by changing/removing it.

## Safety

- No display-object `ToString()` conversion is used by HUD discovery.
- Search is cycle-safe and bounded.
- HUD rebuilds invalidate managed Scaleform objects before teardown.
- Existing fast-travel baseline state remains stored on the live holder.
- Unknown HUDs are left untouched rather than being forced through CNO marker rendering.

## Visual ownership

The normal release does not include generated Compass or QuestItemList SWFs. This is intentional: the installed UI mod owns the visual design.

An optional local adaptive SWF builder is included for compatibility testing. It patches the currently installed SWF's ActionScript while preserving its graphics/timelines/symbols and writes the result under `adaptive-output/`. This output is local/design-specific and is not part of the normal release package.

## Compatibility expectation

Expected to cover Vanilla/CNO layouts, Nordic UI/CNO skins, SkyHUD-style layouts, and similar replacers that retain normal Skyrim/CNO compass data/functions. A total HUD rewrite that removes those semantics can still require a dedicated adapter.
