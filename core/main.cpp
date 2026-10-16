#include "options.h"
#include "package_debug.h"
#include "resource.h"
#include "startup.h"
#include <commctrl.h>
#include <shellapi.h>
#include <exception>

namespace {
constexpr UINT TrayMessage = WM_APP + 1;
const UINT ShowMessage = RegisterWindowMessageW(L"StayAliveMC.Show.7D54BE37");
const UINT TaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

struct ComApartment {
    ComApartment() {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(result)) throw result;
    }
    ~ComApartment() { CoUninitialize(); }
};

struct WindowState {
    stayalive::PackageDebugSession session;
    NOTIFYICONDATAW tray{};
    bool trayAdded = false;
    bool background = false;
};

bool WriteOutput(const std::wstring& message) {
    const int length = WideCharToMultiByte(CP_UTF8, 0, message.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (!length) return false;
    std::string bytes(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, message.c_str(), -1, bytes.data(), length, nullptr, nullptr);
    DWORD written = 0;
    return WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), bytes.data(),
        static_cast<DWORD>(length - 1), &written, nullptr) != FALSE;
}

void ShowWindowAgain(HWND window) {
    ShowWindow(window, SW_RESTORE);
    SetForegroundWindow(window);
}

void UpdateButtons(HWND window, const WindowState& state) {
    EnableWindow(GetDlgItem(window, IDC_ENABLE), !state.session.Active());
    EnableWindow(GetDlgItem(window, IDC_RESTORE), state.session.Active());
}

void ShowError(HWND window, HRESULT error) {
    std::wstring message;
    if (error == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        message = L"Minecraft UWP was not found for this Windows account.\r\n"
                  L"Java, Preview and GDK editions are not supported.";
    } else {
        message = L"Windows could not complete the operation.\r\n" + stayalive::ErrorMessage(error);
    }
    SetDlgItemTextW(window, IDC_STATUS, message.c_str());
    ShowWindowAgain(window);
}

INT_PTR CALLBACK DialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<WindowState*>(GetWindowLongPtrW(window, DWLP_USER));
    try {
        if (message == WM_INITDIALOG) {
            state = reinterpret_cast<WindowState*>(lParam);
            SetWindowLongPtrW(window, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
            const auto icon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP),
                IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
            const auto smallIcon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP),
                IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
            SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
            SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
            state->tray.cbSize = sizeof(state->tray);
            state->tray.hWnd = window;
            state->tray.uID = 1;
            state->tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
            state->tray.uCallbackMessage = TrayMessage;
            state->tray.hIcon = smallIcon;
            wcscpy_s(state->tray.szTip, L"StayAliveMC - open to view status");
            state->trayAdded = Shell_NotifyIconW(NIM_ADD, &state->tray) != FALSE;
            UpdateButtons(window, *state);
            CheckDlgButton(window, IDC_STARTUP, stayalive::StartupEnabled() ? BST_CHECKED : BST_UNCHECKED);
            if (state->background) PostMessageW(window, WM_COMMAND, IDC_ENABLE, 0);
            return TRUE;
        }
        if (!state) return FALSE;
        if (message == ShowMessage) {
            ShowWindowAgain(window);
            return TRUE;
        }
        if (message == TaskbarCreated) {
            state->trayAdded = Shell_NotifyIconW(NIM_ADD, &state->tray) != FALSE;
            if (!state->trayAdded) ShowWindowAgain(window);
            return TRUE;
        }
        switch (message) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDC_ENABLE:
                state->session.Enable();
                SetDlgItemTextW(window, IDC_STATUS,
                    L"Enabled for this session. Minecraft UWP can keep running\r\n"
                    L"when minimized. StayAliveMC must remain running.");
                UpdateButtons(window, *state);
                return TRUE;
            case IDC_RESTORE:
                state->session.Disable();
                SetDlgItemTextW(window, IDC_STATUS, L"Restored. Windows can suspend Minecraft UWP normally.");
                UpdateButtons(window, *state);
                return TRUE;
            case IDC_STARTUP: {
                const bool enabled = IsDlgButtonChecked(window, IDC_STARTUP) == BST_CHECKED;
                try { stayalive::SetStartup(enabled); }
                catch (...) {
                    CheckDlgButton(window, IDC_STARTUP, enabled ? BST_UNCHECKED : BST_CHECKED);
                    throw;
                }
                SetDlgItemTextW(window, IDC_STATUS, enabled
                    ? L"Automatic apply at sign-in enabled. Keep this executable\r\n"
                      L"in its current folder, or re-register after moving it."
                    : L"Automatic apply at sign-in disabled. The current session\r\n"
                      L"is unchanged; use Restore default to stop keeping alive.");
                return TRUE;
            }
            case IDM_OPEN: ShowWindowAgain(window); return TRUE;
            case IDCANCEL:
            case IDM_EXIT: DestroyWindow(window); return TRUE;
            }
            break;
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED && state->trayAdded) ShowWindow(window, SW_HIDE);
            break;
        case TrayMessage:
            if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) ShowWindowAgain(window);
            else if (lParam == WM_RBUTTONUP) {
                HMENU menu = CreatePopupMenu();
                if (!menu) throw HRESULT_FROM_WIN32(GetLastError());
                AppendMenuW(menu, MF_STRING, IDM_OPEN, L"Open StayAliveMC");
                AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit and restore default");
                POINT cursor{};
                GetCursorPos(&cursor);
                SetForegroundWindow(window);
                const UINT command = static_cast<UINT>(TrackPopupMenu(menu,
                    TPM_RETURNCMD | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, window, nullptr));
                DestroyMenu(menu);
                if (command) PostMessageW(window, WM_COMMAND, command, 0);
                PostMessageW(window, WM_NULL, 0, 0);
            }
            return TRUE;
        case WM_CLOSE: DestroyWindow(window); return TRUE;
        case WM_DESTROY:
            if (state->trayAdded) Shell_NotifyIconW(NIM_DELETE, &state->tray);
            PostQuitMessage(0);
            return TRUE;
        }
    } catch (HRESULT error) {
        ShowError(window, error);
        if (state) UpdateButtons(window, *state);
    } catch (const std::exception&) {
        SetDlgItemTextW(window, IDC_STATUS, L"The operation failed. Check the folder and file permissions, then try again.");
        ShowWindowAgain(window);
    } catch (...) {
        SetDlgItemTextW(window, IDC_STATUS, L"An unexpected error occurred. Exit and restart StayAliveMC.");
        ShowWindowAgain(window);
    }
    return FALSE;
}

int Run(HINSTANCE instance) {
    int count = 0;
    auto arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!arguments) return 1;
    const auto mode = stayalive::ParseMode(count, arguments);
    LocalFree(arguments);
    if (mode == stayalive::Mode::Help || mode == stayalive::Mode::Invalid) {
        const std::wstring usage = L"StayAliveMC\r\n\r\n"
            L"No arguments: open the window.\r\n"
            L"--background: enable and keep running in the tray.\r\n"
            L"--check: detect the Minecraft UWP package without changing settings.\r\n"
            L"--help: show this help.\r\n";
        if (!WriteOutput(usage)) MessageBoxW(nullptr, usage.c_str(), L"StayAliveMC", MB_OK);
        return mode == stayalive::Mode::Help ? 0 : 2;
    }
    if (mode == stayalive::Mode::Check) {
        const auto packages = stayalive::FindMinecraftPackages();
        for (const auto& package : packages) WriteOutput(package + L"\r\n");
        if (packages.empty()) WriteOutput(L"Minecraft UWP was not found.\r\n");
        return packages.empty() ? 3 : 0;
    }
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\StayAliveMC.7D54BE37");
    if (!mutex) throw HRESULT_FROM_WIN32(GetLastError());
    const bool alreadyRunning = GetLastError() == ERROR_ALREADY_EXISTS;
    if (alreadyRunning) {
        if (mode == stayalive::Mode::Window) PostMessageW(HWND_BROADCAST, ShowMessage, 0, 0);
        CloseHandle(mutex);
        return 0;
    }
    struct MutexOwner { HANDLE handle; ~MutexOwner() { CloseHandle(handle); } } owner{mutex};
    WindowState state;
    state.background = mode == stayalive::Mode::Background;
    HWND window = CreateDialogParamW(instance, MAKEINTRESOURCEW(IDD_MAIN), nullptr,
        DialogProc, reinterpret_cast<LPARAM>(&state));
    if (!window) throw HRESULT_FROM_WIN32(GetLastError());
    if (!state.background || !state.trayAdded) ShowWindow(window, SW_SHOW);
    MSG message{};
    int result;
    while ((result = static_cast<int>(GetMessageW(&message, nullptr, 0, 0))) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return result == -1 ? 1 : 0;
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    try {
        ComApartment apartment;
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
        if (!InitCommonControlsEx(&controls)) throw HRESULT_FROM_WIN32(GetLastError());
        return Run(instance);
    } catch (HRESULT error) {
        const auto message = L"StayAliveMC could not start.\r\n" + stayalive::ErrorMessage(error);
        if (!WriteOutput(message)) MessageBoxW(nullptr, message.c_str(), L"StayAliveMC", MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxW(nullptr, L"StayAliveMC could not start.", L"StayAliveMC", MB_OK | MB_ICONERROR);
    }
    return 1;
}
