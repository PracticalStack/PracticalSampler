#ifndef AppVersion
  #error AppVersion must be supplied by the installer build script.
#endif

#ifndef DRSVst3Source
  #error DRSVst3Source must point to the built "Practical Sampler.vst3" bundle.
#endif

#ifndef DRSOutputDir
  #define DRSOutputDir "installer-output"
#endif

#ifndef DRSHasStandalone
  #define DRSHasStandalone 0
#endif

#ifndef DRSStandaloneDir
  #define DRSStandaloneDir ""
#endif

#define MyAppName "Practical Sampler"
#define MyAppPublisher "Practical Sampler Project"
#define MyAppExeName "Practical Sampler.exe"

[Setup]
AppId={{7A99E2F9-AE54-4B4A-B71C-8D9E0F61A9B6}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppPublisher={#MyAppPublisher}
UninstallDisplayName={#MyAppName}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir={#DRSOutputDir}
OutputBaseFilename=PracticalSampler-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableDirPage=yes
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
#if DRSHasStandalone
UninstallDisplayIcon={app}\{#MyAppExeName}
#endif

[Types]
Name: "full"; Description: "Full installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full; Flags: fixed
#if DRSHasStandalone
Name: "standalone"; Description: "Standalone app"; Types: full
#endif

[Dirs]
Name: "{commoncf}\VST3"

[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\Decent Rhapsody Studio.vst3"
Type: files; Name: "{app}\Decent Rhapsody Studio.exe"

[Files]
Source: "{#DRSVst3Source}\*"; DestDir: "{commoncf}\VST3\Practical Sampler.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
#if DRSHasStandalone
Source: "{#DRSStandaloneDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: standalone
#endif

[Icons]
#if DRSHasStandalone
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
#endif
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
#if DRSHasStandalone
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent; Components: standalone
#endif
