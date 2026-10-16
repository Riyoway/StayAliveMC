[CmdletBinding()]
param([string]$Destination = (Join-Path $PSScriptRoot '..\build\inno'))
$ErrorActionPreference = 'Stop'

# Pin the official compiler so local and CI installers use the same toolchain.
$version = '7.1.0'
$expectedHash = '0362a383ed217d4c4239b5933866dd96d3eb2102737da92f80f6057a4b40df2f'
$Destination = [IO.Path]::GetFullPath($Destination)
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$download = Join-Path $Destination 'inno-setup.exe'
Invoke-WebRequest -Uri "https://github.com/jrsoftware/issrc/releases/download/is-7_1_0/innosetup-$version-x64.exe" -OutFile $download
if ((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash -ne $expectedHash) {
    throw 'Inno Setup compiler download did not match the pinned SHA-256.'
}
$setup = Start-Process -FilePath $download -ArgumentList @(
    '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', '/NOICONS', '/TASKS=""',
    "/DIR=`"$Destination`""
) -WindowStyle Hidden -Wait -PassThru
if ($setup.ExitCode -ne 0) { throw "Inno Setup installation failed: $($setup.ExitCode)" }
$compiler = Join-Path $Destination 'ISCC.exe'
if (-not (Test-Path -LiteralPath $compiler)) { throw 'ISCC.exe was not installed.' }
Write-Output $compiler
