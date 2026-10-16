#ifndef BinaryPath
  #define BinaryPath "..\build\Release\StayAliveMC.exe"
#endif
#ifndef OutputPath
  #define OutputPath "..\dist"
#endif
#define AppVersion GetStringFileInfo(BinaryPath, "ProductVersion")

[Setup]
AppId={{9C534169-A8A0-470E-BEC4-9785FF61035F}
AppName=StayAliveMC
AppVersion={#AppVersion}
AppPublisher=Riyo
AppPublisherURL=https://github.com/Riyoway/StayAliveMC
AppSupportURL=https://github.com/Riyoway/StayAliveMC/issues
AppUpdatesURL=https://github.com/Riyoway/StayAliveMC/releases
DefaultDirName={localappdata}\Programs\StayAliveMC
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutputPath}
OutputBaseFilename=StayAliveMC-{#AppVersion}-windows-x64-setup
SetupIconFile=..\assets\icon.ico
UninstallDisplayIcon={app}\StayAliveMC.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\LICENSE
AppMutex=Local\StayAliveMC.7D54BE37
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#BinaryPath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\StayAliveMC"; Filename: "{app}\StayAliveMC.exe"
Name: "{autodesktop}\StayAliveMC"; Filename: "{app}\StayAliveMC.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\StayAliveMC.exe"; Description: "{cm:LaunchProgram,StayAliveMC}"; Flags: nowait postinstall skipifsilent

[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ShortcutPath: String;
  Shell, Shortcut: Variant;
begin
  if CurUninstallStep <> usUninstall then Exit;
  ShortcutPath := ExpandConstant('{userstartup}\StayAliveMC.lnk');
  if not FileExists(ShortcutPath) then Exit;
  try
    Shell := CreateOleObject('WScript.Shell');
    Shortcut := Shell.CreateShortcut(ShortcutPath);
    { Preserve startup shortcuts belonging to a separate portable copy. }
    if CompareText(Shortcut.TargetPath, ExpandConstant('{app}\StayAliveMC.exe')) = 0 then
      DeleteFile(ShortcutPath);
  except
    Log('Could not inspect the startup shortcut; leaving it unchanged.');
  end;
end;
