#include "options.h"
#include "package_debug.h"
#include "startup.h"
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckArguments() {
    using stayalive::Mode;
    const wchar_t* arguments[]{L"StayAliveMC.exe", L"--background", L"unexpected"};
    Require(stayalive::ParseMode(1, arguments) == Mode::Window, "default mode");
    Require(stayalive::ParseMode(2, arguments) == Mode::Background, "background mode");
    Require(stayalive::ParseMode(3, arguments) == Mode::Invalid, "extra arguments rejected");
    arguments[1] = L"--check";
    Require(stayalive::ParseMode(2, arguments) == Mode::Check, "read-only check mode");
    arguments[1] = L"--help";
    Require(stayalive::ParseMode(2, arguments) == Mode::Help, "help mode");
    arguments[1] = L"--enable";
    Require(stayalive::ParseMode(2, arguments) == Mode::Invalid, "unknown option rejected");
}

void CheckShortcut() {
    namespace fs = std::filesystem;
    const auto directory = fs::temp_directory_path() /
        (L"StayAliveMC test 日本語 ' " + std::to_wstring(GetCurrentProcessId()));
    Require(fs::create_directory(directory), "unique temporary test directory");
    struct Cleanup {
        fs::path path;
        ~Cleanup() { std::error_code ignored; fs::remove_all(path, ignored); }
    } cleanup{directory};
    const auto shortcut = directory / L"startup.lnk";
    const auto executable = stayalive::ExecutablePath();
    stayalive::WriteStartupShortcut(shortcut, executable);
    Require(fs::exists(shortcut), "shortcut exists");
    Microsoft::WRL::ComPtr<IShellLinkW> link;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link));
    if (FAILED(result)) throw result;
    Microsoft::WRL::ComPtr<IPersistFile> file;
    result = link.As(&file);
    if (FAILED(result)) throw result;
    result = file->Load(shortcut.c_str(), STGM_READ);
    if (FAILED(result)) throw result;
    wchar_t target[32768]{}, arguments[128]{}, workingDirectory[32768]{};
    Require(SUCCEEDED(link->GetPath(target, ARRAYSIZE(target), nullptr, SLGP_RAWPATH)), "read target");
    Require(SUCCEEDED(link->GetArguments(arguments, ARRAYSIZE(arguments))), "read arguments");
    Require(SUCCEEDED(link->GetWorkingDirectory(workingDirectory, ARRAYSIZE(workingDirectory))), "read working directory");
    Require(fs::path(target) == executable, "absolute shortcut target");
    Require(std::wstring(arguments) == L"--background", "startup keeps the process alive");
    Require(fs::path(workingDirectory) == executable.parent_path(), "working directory");
    // Re-registering replaces an existing (including legacy) shortcut.
    stayalive::WriteStartupShortcut(shortcut, executable);
    bool failed = false;
    try { stayalive::WriteStartupShortcut(directory / L"missing" / L"startup.lnk", executable); }
    catch (HRESULT) { failed = true; }
    Require(failed, "shortcut write failures must propagate");
    Require(stayalive::StartupShortcutPath().filename() == L"StayAliveMC.lnk", "legacy startup name retained");
}
}

int wmain(int count, wchar_t** arguments) {
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized)) return 1;
    struct Cleanup { ~Cleanup() { CoUninitialize(); } } cleanup;
    try {
        CheckArguments();
        CheckShortcut();
        const auto packages = stayalive::FindMinecraftPackages();
        for (const auto& package : packages) {
            Require(package.find(L"Microsoft.MinecraftUWP_") == 0, "full name, not family name");
            Require(package.find(L"8wekyb3d8bbwe") != std::wstring::npos, "expected publisher");
        }
        if (count == 2 && std::wstring_view(arguments[1]) == L"--integration") {
            Require(!packages.empty(), "integration test requires Minecraft UWP");
            stayalive::PackageDebugSession session;
            Require(!session.Active(), "initially inactive");
            session.Disable();
            session.Enable();
            Require(session.Active(), "enabled session retained");
            session.Enable();
            session.Disable();
            session.Disable();
            Require(!session.Active(), "restored session");
            { stayalive::PackageDebugSession scoped; scoped.Enable(); }
        }
        std::cout << "All checks passed. Detected " << packages.size() << " Minecraft UWP package(s).\n";
        return 0;
    } catch (HRESULT error) {
        std::wcerr << stayalive::ErrorMessage(error) << L'\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    }
    return 1;
}
