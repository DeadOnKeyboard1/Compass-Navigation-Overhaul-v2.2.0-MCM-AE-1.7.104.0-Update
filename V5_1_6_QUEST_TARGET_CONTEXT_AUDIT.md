# v5.1.6 Quest Target Context Audit

## Why this follow-up was needed

The v5.1.4 Miscellaneous tracking fix correctly stopped rebuilding quest associations from the shared door/location `RefHandle`, but its first implementation assumed RBX at the hooked `HUDMarkerManager::UpdateQuests` call site was always the direct `TESQuestTarget*`. The original CNO hook dereferenced the first qword of that context before comparing it with `BGSQuestObjective::targets`, which means a slot/wrapper layout must also be supported.

## Fix

`UpdateQuests` now receives the sixth hook argument as an opaque context pointer and derives two candidates:

- the context itself, for a direct-target layout;
- the first qword of the context, for the original CNO slot/wrapper layout.

Candidates are tried in that order. A candidate is never used as a `TESQuestTarget` until pointer identity matches one of the target pointers owned by a displayed, running quest objective. Only the verified objective target is passed to `GetTrackingRef()`, and its resolved handle must still equal Skyrim's current marker handle before CNO adds quest metadata. Vanilla `AddMarker` remains authoritative and is preserved even when no CNO target candidate matches.

## v5.1.5 refresh follow-up

The rendered quest-payload fingerprint now also contains `GetObjectiveDisplayText()` output. This covers dynamic radiant/alias/tag substitutions that can change without changing the quest pointer, objective pointer, or instance ID. Objective text is expanded only while the quest-list overlay is ready and allowed to render.

## Intentionally unchanged

- Skyrim 1.7.104.0 hook addresses and relocation IDs
- v5.1.2 interior focus-angle math
- location/enemy/player-set marker hooks
- Alternate Perspective suppression logic
- Compass/QuestItemList ActionScript and SWFs
- ESP
- MCM config/defaults and all 25 setting IDs
- all ten translations
- CommonLibSSE-NG 7.5.4 pin
- build/adaptive-SWF scripts
- no-SWF normal release policy
