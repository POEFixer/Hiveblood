# Hiveblood — POE2Fixer Plugin

Shows your **Hiveblood** total (Path of Exile 2 0.5 Breach / Genesis Tree
resource) as an in-game overlay, read directly from memory — anywhere,
without opening the Genesis Tree.

## What It Does

When enabled, draws a small overlay with your current Hiveblood total and how
much you've gained on the current map. Unlike UI-scraping trackers, it reads
the value straight from game memory, so the total is available on maps, in
your hideout, and at the tree — no need to walk up to the Genesis Tree to
check it.

- **Total anywhere** — read live where Hiveblood changes (at the tree on
  spend, on maps on Breach gain). In idle areas (e.g. a hideout) it shows the
  last value it read, which stays correct because the total only changes at
  the tree or on maps.
- **Per-map gains** — a `+N this map` line that resets each area, so you can
  see how much a map yielded.
- **Cap warning** — the text turns orange-red and blinks above a configurable
  threshold (default 95,000) as you approach the 100k cap.
- Configurable overlay position, font scale, and color from the Plugins
  settings tab inside POE2Fixer.

## Disclaimer

Community third-party plugin for POE2Fixer. Use at your own risk. Not
affiliated with Grinding Gear Games.
