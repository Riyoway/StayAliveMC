#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <string>
#include <vector>

namespace stayalive {
inline constexpr wchar_t PackageFamily[] = L"Microsoft.MinecraftUWP_8wekyb3d8bbwe";
std::wstring ErrorMessage(HRESULT error);
std::vector<std::wstring> FindMinecraftPackages();

class PackageDebugSession {
public:
    PackageDebugSession() = default;
    PackageDebugSession(const PackageDebugSession&) = delete;
    PackageDebugSession& operator=(const PackageDebugSession&) = delete;
    ~PackageDebugSession();
    void Enable();
    void Disable();
    bool Active() const { return !packages_.empty(); }
private:
    Microsoft::WRL::ComPtr<IPackageDebugSettings> settings_;
    std::vector<std::wstring> packages_;
};
}
