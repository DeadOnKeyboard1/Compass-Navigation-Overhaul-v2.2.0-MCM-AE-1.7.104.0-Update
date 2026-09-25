# v4.5 Settings Migration Fix

Target: Skyrim AE 1.7.104.0

Problem fixed:
- Upgrading from older CNO builds could reset the visible Compass layout to the shipped MCM defaults (notably 100% scale).

Cause:
- Runtime load priority was `legacy SKSE INI -> shipped MCM defaults -> MCM user settings`.
- When no MCM user settings file existed, the shipped defaults overwrote legacy layout values.

Fix:
- Runtime priority is now `shipped MCM defaults -> legacy SKSE INI -> MCM user settings`.
- If the MCM user settings file does not exist, the legacy SKSE INI is copied to it once.
- Existing MCM user settings are never overwritten.

No marker, InfinityUI, Fast Travel, MCM registration, translation, or hook behavior was changed by this fix.
