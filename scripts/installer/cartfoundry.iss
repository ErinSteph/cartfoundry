#ifndef PackageRoot
  #error PackageRoot is required
#endif
#ifndef OutputRoot
  #error OutputRoot is required
#endif
#ifndef AppVersion
  #error AppVersion is required
#endif
[Setup]
AppId={{D4963015-2157-4FC7-8ABD-0A102C9DD92A}
AppName=CartFoundry
AppVersion={#AppVersion}
AppPublisher=EmeraldSoftlab
AppPublisherURL=https://eslab.click
AppSupportURL=https://github.com/ErinSteph/cartfoundry
DefaultDirName={localappdata}\Programs\CartFoundry
DefaultGroupName=CartFoundry
DisableProgramGroupPage=yes
DisableDirPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
UninstallFilesDir={localappdata}\CartFoundry\Installer
UninstallDisplayIcon={app}\CartFoundry.exe
OutputDir={#OutputRoot}
OutputBaseFilename=CartFoundry-{#AppVersion}-windows-x86_64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=no
RestartApplications=no
SetupLogging=yes
[Tasks]
Name: desktopicon; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked
[Files]
Source: "{#PackageRoot}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\CartFoundry"; Filename: "{app}\CartFoundry.exe"; WorkingDir: "{userdocs}"
Name: "{autodesktop}\CartFoundry"; Filename: "{app}\CartFoundry.exe"; WorkingDir: "{userdocs}"; Tasks: desktopicon
[Run]
Filename: "{app}\CartFoundry.exe"; Description: "Launch CartFoundry"; Flags: nowait postinstall skipifsilent
