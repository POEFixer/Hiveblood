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

## Building

Open `Hiveblood.sln` in Visual Studio 2022. Build Release|x64. Output:

    bin/Release/Hiveblood.dll

Or from the command line:
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Hiveblood.sln -p:Configuration=Release -p:Platform=x64
```

## Installing

Copy the folder (or just the built DLL) into POE2Fixer's plugin directory:

    POE2Fixer/Plugins/Hiveblood/Hiveblood.dll

Restart POE2Fixer and enable **Hiveblood** in the Plugins tab. Visit a map or
the Genesis Tree once so it reads your current total.

## Source

The same source ships inside the main POE2Fixer repo at `Plugins/Hiveblood/`.
This standalone repo is for plugin authors who want to read it or fork it as a
template for a memory-reading overlay plugin.

## SDK Version

This plugin targets **v6** of the Plugin SDK. The bundled `sdk/` headers must
match the host POE2Fixer's expected SDK version (a run-time check refuses
mismatched plugins with a clear log entry).

## Project Structure

```
sdk/                 Plugin SDK headers (PluginAbi.h, PluginSDK.h)
imgui/               ImGui library (headers + sources, compiled into the DLL)
Hiveblood.cpp        Main plugin entry point (pointer-chain read + overlay)
HivebloodSettings.h  Settings POCO with JSON persistence
```

## Disclaimer

Community third-party plugin for POE2Fixer. Use at your own risk. Not
affiliated with Grinding Gear Games.
