#pragma once
#include <filesystem>

namespace stayalive {
std::filesystem::path ExecutablePath();
std::filesystem::path StartupShortcutPath();
bool StartupEnabled();
void SetStartup(bool enabled);
// Explicit paths let tests check shortcut handling without changing the user's startup.
void WriteStartupShortcut(const std::filesystem::path& shortcut, const std::filesystem::path& executable);
}
