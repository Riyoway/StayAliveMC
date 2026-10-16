[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$version = [Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $root 'build\Release\StayAliveMC.exe')).ProductVersion
$installer = Join-Path $root "dist\StayAliveMC-$version-windows-x64-setup.exe"
$target = Join-Path $root 'build\installer-smoke'
$registration = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{9C534169-A8A0-470E-BEC4-9785FF61035F}_is1'
$programs = [Environment]::GetFolderPath('Programs')
$startup = Join-Path ([Environment]::GetFolderPath('Startup')) 'StayAliveMC.lnk'
$menuLink = Join-Path $programs 'StayAliveMC.lnk'

# This smoke test uses real per-user installation records; never touch an existing install.
foreach ($path in @($registration, $target, $startup, $menuLink)) {
    if (Test-Path -LiteralPath $path) { throw "Smoke test requires a clean installation state: $path" }
}
if (Get-Process -Name StayAliveMC -ErrorAction SilentlyContinue) { throw 'Exit StayAliveMC before the smoke test.' }

function Invoke-Setup([string]$File, [string[]]$Arguments) {
    $process = Start-Process -FilePath $File -ArgumentList $Arguments -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "$File failed with exit code $($process.ExitCode)" }
}
function Install-App {
    Invoke-Setup $installer @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/DIR=`"$target`"", "/LOG=`"$root\build\installer-smoke.log`"")
}
function Uninstall-App {
    Invoke-Setup (Join-Path $target 'unins000.exe') @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART')
}

$shell = New-Object -ComObject WScript.Shell
$testShortcutCreated = $false
try {
    Install-App
    Install-App # Upgrade in place must remain a single, removable installation.
    $installedBinary = Join-Path $target 'StayAliveMC.exe'
    $expectedHash = (Get-FileHash -LiteralPath (Join-Path $root 'build\Release\StayAliveMC.exe')).Hash
    if ((Get-FileHash -LiteralPath $installedBinary).Hash -ne $expectedHash) { throw 'Installed binary differs from the build.' }
    if (-not (Test-Path -LiteralPath $registration)) { throw 'Uninstall registration is missing.' }
    if (-not (Test-Path -LiteralPath $menuLink)) { throw 'Start menu shortcut is missing.' }
    if ($shell.CreateShortcut($menuLink).TargetPath -ne $installedBinary) { throw 'Start menu target is incorrect.' }
    $check = Start-Process -FilePath $installedBinary -ArgumentList '--check' -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $root 'build\installer-check.txt')
    if ($check.ExitCode -notin @(0,3)) { throw "Installed app failed to start: $($check.ExitCode)" }

    $link = $shell.CreateShortcut($startup)
    $link.TargetPath = $installedBinary
    $link.Arguments = '--background'
    $link.Save()
    $testShortcutCreated = $true
    Uninstall-App
    if (Test-Path -LiteralPath $startup) { throw 'Uninstall left its startup shortcut behind.' }
    if (Test-Path -LiteralPath $registration) { throw 'Uninstall registration was not removed.' }
    if (Test-Path -LiteralPath $menuLink) { throw 'Start menu shortcut was not removed.' }
    if (Test-Path -LiteralPath $target) { throw 'Installation directory was not removed.' }

    Install-App
    $portableBinary = Join-Path $root 'build\Release\StayAliveMC.exe'
    $link = $shell.CreateShortcut($startup)
    $link.TargetPath = $portableBinary
    $link.Save()
    Uninstall-App
    if (-not (Test-Path -LiteralPath $startup)) { throw 'Uninstall removed the separate portable startup shortcut.' }
    if ($shell.CreateShortcut($startup).TargetPath -ne $portableBinary) { throw 'Portable startup target was changed.' }
    Write-Output 'Installer smoke test passed: install, upgrade, launch, uninstall, and startup ownership.'
} finally {
    $uninstaller = Join-Path $target 'unins000.exe'
    if (Test-Path -LiteralPath $uninstaller) { Uninstall-App }
    if ($testShortcutCreated -and (Test-Path -LiteralPath $startup)) {
        Remove-Item -LiteralPath $startup
    }
}
