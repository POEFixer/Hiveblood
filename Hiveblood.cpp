// File: Plugins/Hiveblood/src/Hiveblood.cpp
//
// Hiveblood tracker plugin (SDK v6).
//
// Reads the Hiveblood total (PoE2 0.5 Breach / Genesis Tree resource) directly
// from game memory via a fixed pointer chain and shows it as a small overlay,
// plus how much was gained on the current map. Unlike the community GameHelper2
// "Hiveblood" plugin (which scrapes the Genesis Tree UI text + Breach popups),
// this reads the live value straight from memory.
//
// Pointer chain (validated against PoE2 0.5.3; re-resolved to the field across a
// relog AND a full client restart, so every offset is structural):
//   root  = PathOfExile.exe + 0x4511468            (== "Game States" static - 0x10)
//   cur   = [root]
//   cur   = [cur + 0x1C8] -> [+0x60] -> [+0xD0] -> [+0x8] -> [+0x148]
//   total = int32 at [cur + 0x688]
//
// SCOPE / caching: this chain lands in a Breach/Genesis controller copy that is
// correct exactly where Hiveblood CHANGES -- at the Genesis Tree (on spend) and
// on maps (on Breach gain) -- but reads 0 in idle areas like a hideout (the
// controller is not populated there). Since the total cannot change in those
// areas, the plugin caches the last good reading and shows it where the live
// read is 0. The cached value is therefore always correct; an always-resident
// account copy exists (confirmed in memory) but has no unique anchor and would
// need a much larger pointer-scan to pin, with no functional benefit over the
// cache.
//
// PLUGIN_EXPORTS is set in the vcxproj; PluginSDK.h then emits the
// PluginSDK_AttachHost export and makes PLUGIN_API = __declspec(dllexport).

#include "sdk/PluginSDK.h"

#include "HivebloodSettings.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace {

// Module-relative root of the Hiveblood pointer chain (PoE2 0.5.3 build). Equals
// the "Game States" pattern target minus 0x10; both roots are tried.
constexpr uintptr_t kHivebloodChainRva = 0x4511468;
constexpr uintptr_t kDerefOffsets[]    = {0x1C8, 0x60, 0xD0, 0x8, 0x148};
constexpr uintptr_t kFieldOffset       = 0x688;
constexpr int32_t   kHivebloodCap      = 100000;

inline bool IsUserPointer(uint64_t p) {
    return p >= 0x10000 && p < 0x7FFFFFFFFFFFull;
}

// "10344" -> "10,344".
std::string FormatThousands(int32_t n) {
    std::string s = std::to_string(n);
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(i, ",");
    return s;
}

}  // namespace

class HivebloodPlugin : public PluginSDK::Plugin {
public:
    const char* GetName() const override { return "Hiveblood"; }
    bool        WantsOverlay() const override { return true; }

    void OnEnable(bool /*isGameAttached*/) override {
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));
        m_s.Load(DirectoryPath());
        ctx()->Log.Info("Hiveblood v6 enabled");
    }

    void OnDisable() override {
        m_s.Save(DirectoryPath());
        ctx()->Log.Info("Hiveblood v6 disabled");
    }

    void SaveSettings() override { m_s.Save(DirectoryPath()); }

    void DrawSettings() override {
        ImGui::TextWrapped(
            "Reads your Hiveblood total directly from memory and shows it as an "
            "overlay, with how much you gained on the current map. The total is "
            "shown everywhere; in idle areas (e.g. a hideout) it shows the last "
            "value it read, which stays correct because Hiveblood only changes at "
            "the tree or on maps.");
        ImGui::Separator();
        ImGui::Checkbox("Show overlay", &m_s.ShowOverlay);
        ImGui::DragFloat2("Screen position", m_s.Pos, 1.f, 0.f, 8000.f, "%.0f");
        ImGui::SliderFloat("Font scale", &m_s.FontScale, 0.5f, 2.5f, "%.2f");
        ImGui::ColorEdit4("Text color", &m_s.TextColor.x);
        ImGui::Checkbox("Warn near cap", &m_s.WarnNearCap);
        ImGui::SliderInt("Warn from", &m_s.WarnThreshold, 50000, 100000);
        ImGui::Checkbox("Show gains this map", &m_s.ShowMapGains);

        ImGui::Separator();
        int32_t v = 0;
        if (ReadHiveblood(v) && v > 0)
            ImGui::Text("Current: %s", FormatThousands(v).c_str());
        else if (m_hasCached)
            ImGui::Text("Current: %s (last seen)", FormatThousands(m_cachedTotal).c_str());
        else
            ImGui::TextDisabled("Current: not read yet (visit a map or the tree)");
    }

    void DrawUI() override {
        if (!ctx()->Game.IsInGame()) return;
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));
        if (!m_s.ShowOverlay) return;

        // Reset the per-map baseline on every area transition so the gain line
        // counts what was earned on THIS map, not the whole total.
        const uint32_t area = ReadAreaCounter();
        if (area != m_lastArea) { m_lastArea = area; m_hasBaseline = false; }

        // Live read where the chain is populated (tree / maps); otherwise fall
        // back to the last good reading (correct, since the total can't change
        // where the chain reads 0).
        int32_t live = 0;
        const bool gotLive = ReadHiveblood(live) && live > 0;

        int32_t total  = 0;
        bool    cached = false;
        if (gotLive) {
            m_cachedTotal = live;
            m_hasCached   = true;
            total = live;
            if (!m_hasBaseline) { m_baseline = live; m_hasBaseline = true; }
        } else if (m_hasCached) {
            total  = m_cachedTotal;
            cached = true;
        } else {
            return;  // nothing read yet this session (e.g. logged straight into a hideout)
        }

        const int32_t mapGain = (gotLive && total > m_baseline) ? (total - m_baseline) : 0;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;
        ImFont* font = ImGui::GetFont();
        const float scale = std::clamp(m_s.FontScale, 0.5f, 2.5f);
        const float size  = ImGui::GetFontSize() * scale;

        ImVec4 col = m_s.TextColor;
        if (m_s.WarnNearCap && total >= m_s.WarnThreshold) {
            col = ImVec4(1.f, 0.45f, 0.30f, col.w);
            const bool on = std::fmod(ImGui::GetTime(), 1.0) < 0.5;  // ~1 Hz blink
            col.w *= on ? 1.f : 0.4f;
        }
        if (cached) col.w *= 0.65f;  // dim slightly: this is the last-seen value

        const ImU32 textCol   = ImGui::ColorConvertFloat4ToU32(col);
        const ImU32 shadowCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 0.85f * col.w));
        const ImVec2 pos(m_s.Pos[0], m_s.Pos[1]);

        const std::string line1 = "Hiveblood: " + FormatThousands(total);
        DrawShadow(dl, font, size, pos, shadowCol, textCol, line1.c_str());

        if (m_s.ShowMapGains && mapGain > 0) {
            const std::string line2 = "(+" + FormatThousands(mapGain) + " this map)";
            const ImVec2 pos2(pos.x, pos.y + size + 2.f);
            const ImU32 dim = ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, col.w * 0.8f));
            DrawShadow(dl, font, size * 0.8f, pos2, shadowCol, dim, line2.c_str());
        }
    }

private:
    static void DrawShadow(ImDrawList* dl, ImFont* font, float size, ImVec2 pos,
                           ImU32 shadow, ImU32 text, const char* s) {
        dl->AddText(font, size, ImVec2(pos.x + 1.f, pos.y + 1.f), shadow, s);
        dl->AddText(font, size, pos, text, s);
    }

    // Area-transition counter (uint32). Cached after first resolve; the global's
    // address is static for the process lifetime.
    uint32_t ReadAreaCounter() {
        if (!m_areaCounterAddr)
            m_areaCounterAddr = ctx()->Memory.GetPatternAddress("AreaChangeCounter");
        uint32_t c = 0;
        if (m_areaCounterAddr) ctx()->Memory.Read(m_areaCounterAddr, &c, sizeof(c));
        return c;
    }

    // Resolve the pointer chain and read the Hiveblood total. Returns false if
    // any link faults (host RPM is SEH-guarded and returns false) or the value
    // is outside the valid range -- a chain broken by a game patch yields
    // garbage, which we refuse to display.
    bool ReadHiveblood(int32_t& out) const {
        const auto* c = ctx();
        auto walk = [&](uintptr_t root) -> bool {
            if (!root) return false;
            uint64_t cur = 0;
            if (!c->Memory.Read(root, &cur, sizeof(cur)) || !IsUserPointer(cur)) return false;
            for (uintptr_t off : kDerefOffsets) {
                if (!c->Memory.Read(cur + off, &cur, sizeof(cur)) || !IsUserPointer(cur)) return false;
            }
            int32_t v = 0;
            if (!c->Memory.Read(cur + kFieldOffset, &v, sizeof(v))) return false;
            if (v < 0 || v > kHivebloodCap) return false;
            out = v;
            return true;
        };
        const uintptr_t base = c->Memory.GetBaseAddress();
        if (base && walk(base + kHivebloodChainRva)) return true;
        const uintptr_t gs = c->Memory.GetPatternAddress("Game States");
        if (gs && walk(gs - 0x10)) return true;
        return false;
    }

    HivebloodSettings m_s;
    uint32_t  m_lastArea        = 0xFFFFFFFFu;  // sentinel: first frame triggers a reset
    int32_t   m_baseline        = 0;            // total on entering the current map
    bool      m_hasBaseline     = false;
    int32_t   m_cachedTotal     = 0;            // last good reading (shown where chain reads 0)
    bool      m_hasCached       = false;
    uintptr_t m_areaCounterAddr = 0;            // cached AreaChangeCounter address
};

// ----------------------------------------------------------------------------
// v6 factory exports
// ----------------------------------------------------------------------------

extern "C" PLUGIN_API PluginSDK::Plugin* CreatePlugin() {
    return new HivebloodPlugin();
}

extern "C" PLUGIN_API void DestroyPlugin(PluginSDK::Plugin* p) {
    delete p;
}
