#include "package_debug.h"
#include <appmodel.h>
#include <iomanip>
#include <sstream>

namespace stayalive {
std::wstring ErrorMessage(HRESULT error) {
    wchar_t buffer[1024]{};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(error), 0, buffer, ARRAYSIZE(buffer), nullptr);
    std::wostringstream message;
    message << buffer << L" (0x" << std::hex << std::uppercase
            << std::setw(8) << std::setfill(L'0') << static_cast<unsigned long>(error) << L")";
    return message.str();
}

std::vector<std::wstring> FindMinecraftPackages() {
    // The package list can change while the Store installs an update.
    for (int attempt = 0; attempt < 3; ++attempt) {
        UINT32 count = 0, length = 0;
        LONG result = FindPackagesByPackageFamily(PackageFamily, PACKAGE_FILTER_HEAD,
            &count, nullptr, &length, nullptr, nullptr);
        if (result == ERROR_SUCCESS && count == 0) return {};
        if (result != ERROR_INSUFFICIENT_BUFFER) throw HRESULT_FROM_WIN32(result);
        std::vector<PWSTR> names(count);
        std::vector<wchar_t> buffer(length);
        result = FindPackagesByPackageFamily(PackageFamily, PACKAGE_FILTER_HEAD,
            &count, names.data(), &length, buffer.data(), nullptr);
        if (result == ERROR_INSUFFICIENT_BUFFER) continue;
        if (result != ERROR_SUCCESS) throw HRESULT_FROM_WIN32(result);
        std::vector<std::wstring> packages;
        for (UINT32 index = 0; index < count; ++index) packages.emplace_back(names[index]);
        return packages;
    }
    throw HRESULT_FROM_WIN32(ERROR_RETRY);
}

PackageDebugSession::~PackageDebugSession() {
    try { Disable(); } catch (...) { /* Releasing the interface also clears its options. */ }
}

void PackageDebugSession::Enable() {
    if (Active()) return;
    auto packages = FindMinecraftPackages();
    if (packages.empty()) throw HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    if (!settings_) {
        const HRESULT result = CoCreateInstance(CLSID_PackageDebugSettings, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(settings_.GetAddressOf()));
        if (FAILED(result)) throw result;
    }
    // Store names before changing Windows settings so allocation cannot lose ownership.
    packages_ = std::move(packages);
    for (const auto& package : packages_) {
        const HRESULT result = settings_->EnableDebugging(package.c_str(), nullptr, nullptr);
        if (FAILED(result)) {
            try { Disable(); } catch (...) { settings_.Reset(); packages_.clear(); }
            throw result;
        }
    }
}

void PackageDebugSession::Disable() {
    HRESULT firstError = S_OK;
    for (auto entry = packages_.begin(); entry != packages_.end();) {
        const HRESULT result = settings_->DisableDebugging(entry->c_str());
        if (SUCCEEDED(result)) entry = packages_.erase(entry);
        else {
            if (SUCCEEDED(firstError)) firstError = result;
            ++entry;
        }
    }
    if (FAILED(firstError)) throw firstError;
    settings_.Reset();
}
}
