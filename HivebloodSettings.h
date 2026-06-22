// File: Plugins/Hiveblood/src/HivebloodSettings.h
//
// Settings POCO for the Hiveblood tracker plugin (SDK v6).
//
// Persists to <PluginDirectory>/config/settings.json. JSON is hand-rolled (no
// nlohmann dependency) so the plugin DLL stays self-contained. Save/Load take a
// std::filesystem::path (pass DirectoryPath(), which is Unicode-safe) rather
// than a std::string, because fs::path(std::string) on Windows interprets bytes
// via the ANSI codepage and mangles non-ASCII install dirs (Cyrillic/CJK).

#pragma once

#include <imgui.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

struct HivebloodSettings {
    bool   ShowOverlay      = true;
    float  Pos[2]           = {40.f, 200.f};               // screen x,y
    float  FontScale        = 1.15f;
    ImVec4 TextColor        = {0.78f, 0.45f, 0.95f, 1.f};  // amethyst, like the community plugin
    bool   WarnNearCap      = true;
    int    WarnThreshold    = 95000;                       // blink orange-red above this
    bool   ShowMapGains     = true;                        // show "+N this map"

    void Save(const std::filesystem::path& directory) const;
    void Load(const std::filesystem::path& directory);
};

inline void HivebloodSettings::Save(const std::filesystem::path& directory) const {
    std::filesystem::path p = directory / "config" / "settings.json";
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream out(p);
    if (!out.is_open()) return;
    out << "{\n";
    out << "  \"ShowOverlay\": "      << (ShowOverlay ? "true" : "false") << ",\n";
    out << "  \"PosX\": "             << Pos[0]        << ",\n";
    out << "  \"PosY\": "             << Pos[1]        << ",\n";
    out << "  \"FontScale\": "        << FontScale     << ",\n";
    out << "  \"ColorR\": "           << TextColor.x   << ",\n";
    out << "  \"ColorG\": "           << TextColor.y   << ",\n";
    out << "  \"ColorB\": "           << TextColor.z   << ",\n";
    out << "  \"ColorA\": "           << TextColor.w   << ",\n";
    out << "  \"WarnNearCap\": "      << (WarnNearCap ? "true" : "false") << ",\n";
    out << "  \"WarnThreshold\": "    << WarnThreshold << ",\n";
    out << "  \"ShowMapGains\": "     << (ShowMapGains ? "true" : "false") << "\n";
    out << "}\n";
}

inline void HivebloodSettings::Load(const std::filesystem::path& directory) {
    std::filesystem::path p = directory / "config" / "settings.json";
    if (!std::filesystem::exists(p)) return;
    std::ifstream in(p);
    if (!in.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());

    auto rawVal = [&](const char* key) -> std::string {
        std::string k = std::string("\"") + key + "\"";
        size_t pos = content.find(k);
        if (pos == std::string::npos) return std::string();
        pos = content.find(':', pos + k.size());
        if (pos == std::string::npos) return std::string();
        ++pos;
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) ++pos;
        size_t end = content.find_first_of(",}\r\n", pos);
        if (end == std::string::npos) end = content.size();
        return content.substr(pos, end - pos);
    };
    auto readBool  = [&](const char* k, bool def)  { std::string v = rawVal(k); return v.empty() ? def : (v.find("true") != std::string::npos); };
    auto readFloat = [&](const char* k, float def) { std::string v = rawVal(k); if (v.empty()) return def; try { return std::stof(v); } catch (...) { return def; } };
    auto readInt   = [&](const char* k, int def)   { std::string v = rawVal(k); if (v.empty()) return def; try { return std::stoi(v); } catch (...) { return def; } };

    ShowOverlay      = readBool ("ShowOverlay",      ShowOverlay);
    Pos[0]           = readFloat("PosX",             Pos[0]);
    Pos[1]           = readFloat("PosY",             Pos[1]);
    FontScale        = readFloat("FontScale",        FontScale);
    TextColor.x      = readFloat("ColorR",           TextColor.x);
    TextColor.y      = readFloat("ColorG",           TextColor.y);
    TextColor.z      = readFloat("ColorB",           TextColor.z);
    TextColor.w      = readFloat("ColorA",           TextColor.w);
    WarnNearCap      = readBool ("WarnNearCap",      WarnNearCap);
    WarnThreshold    = readInt  ("WarnThreshold",    WarnThreshold);
    ShowMapGains     = readBool ("ShowMapGains",     ShowMapGains);
}
