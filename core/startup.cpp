#include "startup.h"
#include <windows.h>
#include <shlobj.h>
#include <wrl/client.h>
#include <vector>

namespace stayalive {
std::filesystem::path ExecutablePath() {
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!length) throw HRESULT_FROM_WIN32(GetLastError());
    if (length >= buffer.size()) throw HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    return std::wstring(buffer.data(), length);
}

std::filesystem::path StartupShortcutPath() {
    PWSTR directory = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_Startup, KF_FLAG_DEFAULT, nullptr, &directory);
    if (FAILED(result)) throw result;
    std::filesystem::path path;
    try { path = directory; } catch (...) { CoTaskMemFree(directory); throw; }
    CoTaskMemFree(directory);
    return path / L"StayAliveMC.lnk";
}

bool StartupEnabled() { return std::filesystem::exists(StartupShortcutPath()); }

void WriteStartupShortcut(const std::filesystem::path& shortcut, const std::filesystem::path& executable) {
    Microsoft::WRL::ComPtr<IShellLinkW> link;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link));
    if (FAILED(result)) throw result;
    result = link->SetPath(executable.c_str());
    if (FAILED(result)) throw result;
    result = link->SetArguments(L"--background");
    if (FAILED(result)) throw result;
    result = link->SetWorkingDirectory(executable.parent_path().c_str());
    if (FAILED(result)) throw result;
    result = link->SetDescription(L"Keep Minecraft UWP running when minimized");
    if (FAILED(result)) throw result;
    Microsoft::WRL::ComPtr<IPersistFile> file;
    result = link.As(&file);
    if (FAILED(result)) throw result;
    result = file->Save(shortcut.c_str(), TRUE);
    if (FAILED(result)) throw result;
}

void SetStartup(bool enabled) {
    const auto shortcut = StartupShortcutPath();
    if (enabled) WriteStartupShortcut(shortcut, ExecutablePath());
    else std::filesystem::remove(shortcut);
}
}
