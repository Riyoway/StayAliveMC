[CmdletBinding()]
param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '..\build'),
    [string]$Compiler = (Join-Path $PSScriptRoot '..\build\inno\ISCC.exe')
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$binary = [IO.Path]::GetFullPath((Join-Path $BuildDirectory 'Release\StayAliveMC.exe'))
$Compiler = [IO.Path]::GetFullPath($Compiler)
if (-not (Test-Path -LiteralPath $binary)) { throw 'Build the Release configuration first.' }
if (-not (Test-Path -LiteralPath $Compiler)) { throw 'Run scripts/install-inno.ps1 first, or pass -Compiler.' }
$version = [Diagnostics.FileVersionInfo]::GetVersionInfo($binary).ProductVersion
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw "Invalid product version: $version" }
$output = Join-Path $root 'dist'
New-Item -ItemType Directory -Path $output -Force | Out-Null
& $Compiler "/DBinaryPath=$binary" "/DOutputPath=$output" (Join-Path $root 'installer\StayAliveMC.iss')
if ($LASTEXITCODE -ne 0) { throw "Installer compilation failed: $LASTEXITCODE" }
$installer = Join-Path $output "StayAliveMC-$version-windows-x64-setup.exe"
if (-not (Test-Path -LiteralPath $installer)) { throw 'Installer output is missing.' }
$bundle = Join-Path $output "StayAliveMC-$version-windows-x64-portable.zip"
Compress-Archive -LiteralPath $binary,(Join-Path $root 'README.md'),(Join-Path $root 'LICENSE') -DestinationPath $bundle -Force
$checksums = foreach ($artifact in @($installer, $bundle)) {
    $hash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $([IO.Path]::GetFileName($artifact))"
}
[IO.File]::WriteAllLines((Join-Path $output 'SHA256SUMS.txt'), $checksums, [Text.UTF8Encoding]::new($false))
Get-Item -LiteralPath $installer,$bundle,(Join-Path $output 'SHA256SUMS.txt') | Select-Object Name,Length
