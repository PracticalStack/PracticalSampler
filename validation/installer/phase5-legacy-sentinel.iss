#ifndef DRSCurrentExe
  #error DRSCurrentExe must point to the current standalone executable.
#endif
#ifndef DRSCurrentVst3
  #error DRSCurrentVst3 must point to the current VST3 bundle.
#endif
#ifndef DRSOutputDir
  #error DRSOutputDir must point to the validation output directory.
#endif

[Setup]
AppId={{E1F7645D-F4A5-4180-96CB-C4BBECF8A645}
AppName=Practical Sampler Legacy Qualification Sentinel
AppVersion=1
DefaultDirName={tmp}\PracticalSamplerLegacyQualification
OutputDir={#DRSOutputDir}
OutputBaseFilename=PracticalSampler-Legacy-Sentinel
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Uninstallable=no
CreateAppDir=no
Compression=lzma2
SolidCompression=yes

[Files]
Source: "{#DRSCurrentExe}"; DestDir: "{autopf}\Practical Sampler"; DestName: "Decent Rhapsody Studio.exe"; Flags: ignoreversion
Source: "{#DRSCurrentVst3}\*"; DestDir: "{commoncf}\VST3\Decent Rhapsody Studio.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
