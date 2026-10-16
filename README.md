# StayAliveMC

A lightweight Windows utility that keeps **Minecraft Bedrock's UWP edition** running when you minimize the game or switch to another app.

StayAliveMC disables Windows' automatic suspension for the Minecraft UWP package, helping avoid the pauses and multiplayer disconnects caused by suspension. It is a native C++ desktop app: one executable, no Electron, browser engine, Node.js, or separately installed Visual C++ runtime.

## Compatibility

- Windows 10 or Windows 11, x64.
- Minecraft's **UWP edition** installed for the current Windows account, with package family `Microsoft.MinecraftUWP_8wekyb3d8bbwe`.
- Java Edition, Minecraft Preview, and the GDK edition are outside this project's scope. This tool targets UWP lifecycle behavior; it is not a general fix for every Minecraft for Windows version.

This does not prevent server AFK kicks, network failures, system sleep, or pauses implemented by the game itself. A successful apply confirms that Windows accepted the setting, not that a specific server connection will stay alive. Keeping a game running in the background can increase power use.

## Download and install

Get the latest build from [GitHub Releases](https://github.com/Riyoway/StayAliveMC/releases/latest):

- **`StayAliveMC-<version>-windows-x64-setup.exe`**: installs for the current user without administrator privileges, creates a Start menu shortcut, and adds an entry to Windows' installed apps. A desktop shortcut is optional.
- **`StayAliveMC-<version>-windows-x64-portable.zip`**: extract to a permanent folder and run the executable directly.
- **`SHA256SUMS.txt`**: SHA-256 hashes for both downloads.

Exit StayAliveMC before upgrading or uninstalling. Uninstalling removes the startup shortcut if it points to the installed copy; a shortcut belonging to a separate portable copy is preserved. Automatic apply at sign-in is configured inside the app and is off by default for a new user.

The application and installer are currently unsigned.

## Usage

1. Put `StayAliveMC.exe` in a permanent folder and open it.
2. Click **Enable** before minimizing Minecraft. If Minecraft was already suspended, bring it to the foreground first.
3. Keep StayAliveMC running while you play. Minimizing its window hides it in the notification area. Click the tray icon to reopen it, or right-click for the menu.
4. Click **Restore default** to allow normal suspension again. **Exit**, including the window's close button, also restores the settings held by this process.

**Apply automatically at Windows sign-in** creates a shortcut for the current user. At sign-in the app applies the setting and stays in the tray. An apply error opens the window with details. Unchecking this option removes the shortcut without changing the current session; exiting does not unregister startup.

After moving the executable, open it at the new location and check the startup option again to update the shortcut. After a Minecraft package update, restart StayAliveMC and enable it again so it resolves the new full package name.

The app runs with the current user's permissions and does not automatically request elevation. If an operation fails, the window shows the Windows error and its HRESULT.

### Command-line options

| Option | Behavior |
| --- | --- |
| No arguments | Open the control window. |
| `--background` | Enable the setting and keep running in the tray. Used by the startup shortcut. |
| `--check` | Detect the installed Minecraft UWP package without changing settings. |
| `--help`, `-h` | Show usage. |

Exit codes: `0` for success, `1` for an error, `2` for invalid arguments, and `3` when `--check` finds no matching package. Because this is a Windows GUI executable, use `Start-Process -Wait -PassThru` in PowerShell when you need to wait for it or inspect its exit code; redirect standard output if you want the `--check` result in a file.

## Build from source

Install Visual Studio 2022 or newer (or Build Tools) with **Desktop development with C++**, a Windows 10/11 SDK, and CMake 3.21 or newer. No npm dependencies are needed.

```powershell
git clone https://github.com/Riyoway/StayAliveMC.git
cd StayAliveMC
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The standalone executable is `build/Release/StayAliveMC.exe`. The C++ runtime is linked statically. You can also open the repository as a CMake project in Visual Studio.

### Build the installer

The packaging scripts use Inno Setup 7.1.0, downloaded from its official release and verified against a pinned SHA-256:

```powershell
./scripts/install-inno.ps1
./scripts/package.ps1
```

This creates a versioned installer, portable ZIP, and `SHA256SUMS.txt` under `dist/`. To use an existing Inno Setup compiler, pass `-Compiler 'C:\path\to\ISCC.exe'` to `package.ps1`.

The Windows CI workflow builds and tests the app and installer, then uploads the same files as a `StayAliveMC-windows-x64` artifact.

### Tests

The default tests check command-line parsing, read-only package discovery, and real Windows shortcut creation/loading in a temporary directory, including paths with spaces, Unicode, and apostrophes. They do not alter your startup registration or Minecraft settings.

An optional integration test briefly enables and restores the setting on the installed Minecraft UWP package, including repeated calls and cleanup on scope exit:

```powershell
.\build\Release\stayalive_tests.exe --integration
```

Run this with Minecraft closed. Actual minimized gameplay and multiplayer behavior must be checked with your own supported game version.

`./scripts/test-installer.ps1` additionally verifies silent installation, upgrade, launch, uninstall, and startup shortcut cleanup. It requires a clean user installation state and refuses to run if an existing installation or StayAliveMC shortcut is present.

## How it works

The app resolves the installed package's full name using [`FindPackagesByPackageFamily`](https://learn.microsoft.com/en-us/windows/win32/api/appmodel/nf-appmodel-findpackagesbypackagefamily), then calls [`IPackageDebugSettings::EnableDebugging`](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ipackagedebugsettings-enabledebugging) without attaching a debugger.

Windows documents that this disables automatic suspension and changes other package lifecycle behavior, including automatic termination and resumption. The app holds the COM interface for the entire enabled session and calls `DisableDebugging` when restoring or exiting. The [interface documentation](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ipackagedebugsettings) states that its debug options last until cleared or the interface is released. A one-shot helper that exits immediately cannot reliably maintain this setting.

No game files, worlds, or network traffic are modified. Startup uses the Windows Shell shortcut API instead of constructing a PowerShell command.

## Source layout

```text
CMakeLists.txt          Build and test entry point
assets/icon.png        Original application icon
assets/icon.ico        Multi-resolution Windows icon
core/main.cpp          Native window, tray, and process lifetime
core/package_debug.*   Package discovery and debug session ownership
core/startup.*         Current-user startup shortcut
core/options.h         Command-line parsing
core/app.*             Dialog resources and application manifest
tests/tests.cpp        Windows tests and optional integration check
installer/             Per-user Windows installer definition
scripts/               Compiler setup, packaging, and installer smoke test
.github/workflows/     Windows build and test workflow
```

Version 2 replaces the former Electron launcher and detached `svcdata.exe` helper with a single native application. Core lives inside this repository and is built directly from source. There are no sibling repository paths or prebuilt helper binaries to synchronize.

## License

[MIT](LICENSE). An independent community tool, not affiliated with or endorsed by Mojang or Microsoft. Minecraft is a trademark of its respective owners.
