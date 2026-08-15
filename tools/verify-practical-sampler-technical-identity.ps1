[CmdletBinding()]
param(
    [string]$BaselinePath,
    [string]$BuildConfiguration = 'Debug'
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($BaselinePath)) {
    $BaselinePath = Join-Path (Split-Path $PSScriptRoot -Parent) 'docs\practical-sampler-technical-identity-baseline.json'
}
$repositoryRoot = (Resolve-Path (Split-Path $PSScriptRoot -Parent)).Path
$baselineFile = (Resolve-Path -LiteralPath $BaselinePath).Path
$baseline = Get-Content -LiteralPath $baselineFile -Raw | ConvertFrom-Json
$failures = [System.Collections.Generic.List[string]]::new()

function Require-Text {
    param(
        [Parameter(Mandatory)] [string]$RelativePath,
        [Parameter(Mandatory)] [string]$Expected,
        [Parameter(Mandatory)] [string]$Label
    )

    $path = Join-Path $repositoryRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        $script:failures.Add("$Label source is missing: $RelativePath")
        return
    }
    $content = Get-Content -LiteralPath $path -Raw
    if (-not $content.Contains($Expected)) {
        $script:failures.Add("$Label changed or is missing from ${RelativePath}: $Expected")
    }
}

Require-Text 'CMakeLists.txt' ("project({0} " -f $baseline.cmakeIdentity.projectName) 'CMake project name'
Require-Text 'app\CMakeLists.txt' ("juce_add_gui_app({0}" -f $baseline.cmakeIdentity.applicationTarget) 'application target'
Require-Text 'app\CMakeLists.txt' ("juce_add_plugin({0}" -f $baseline.cmakeIdentity.pluginTarget) 'plug-in target'
Require-Text 'app\CMakeLists.txt' ("add_custom_target({0}" -f $baseline.cmakeIdentity.compatibilityTarget) 'compatibility target'
Require-Text 'app\CMakeLists.txt' ("BUNDLE_ID `"{0}`"" -f $baseline.applicationIdentity.bundleId) 'application bundle ID'
Require-Text 'app\CMakeLists.txt' ("BUNDLE_ID `"{0}`"" -f $baseline.pluginIdentity.bundleId) 'plug-in bundle ID'
Require-Text 'app\CMakeLists.txt' ("PLUGIN_MANUFACTURER_CODE {0}" -f $baseline.pluginIdentity.manufacturerCodeFourCc) 'plug-in manufacturer code'
Require-Text 'app\CMakeLists.txt' ("PLUGIN_CODE {0}" -f $baseline.pluginIdentity.pluginCodeFourCc) 'plug-in code'
Require-Text 'engine_adapter\cmake\HisePluginFrontendAppConfig.h.in' ("JucePlugin_ManufacturerCode {0}" -f $baseline.pluginIdentity.frontendTemplateManufacturerCodeHex) 'frontend template manufacturer code'
Require-Text 'engine_adapter\cmake\HisePluginFrontendAppConfig.h.in' ("JucePlugin_PluginCode {0}" -f $baseline.pluginIdentity.frontendTemplatePluginCodeHex) 'frontend template plug-in code'

foreach ($format in $baseline.nativeFormats) {
    $found = & rg --fixed-strings --quiet --glob '!build/**' --glob '!third_party/**' -- $format $repositoryRoot
    if ($LASTEXITCODE -ne 0) {
        $failures.Add("Native format identifier is missing: $format")
    }
}

foreach ($schema in $baseline.coreSchemas) {
    $found = & rg --fixed-strings --quiet --glob '!build/**' --glob '!third_party/**' -- $schema.name $repositoryRoot
    if ($LASTEXITCODE -ne 0) {
        $failures.Add("Core schema identifier is missing: $($schema.name)")
    }
}

Require-Text 'engine_adapter\src\PackageReader.cpp' "'D', 'R', 'S', 'P', 'K', 'G', '1', '\0'" 'package reader magic'
Require-Text 'engine_adapter\src\PackageWriter.cpp' "'D', 'R', 'S', 'P', 'K', 'G', '1', '\0'" 'package writer magic'
Require-Text 'engine_adapter\include\drs\engine\PerformancePackage.h' $baseline.packageIdentity.compatibilityPolicyId 'package compatibility policy ID'
Require-Text 'engine_adapter\src\PackageReader.cpp' $baseline.packageIdentity.payloadDomain 'package payload domain'
Require-Text 'engine_adapter\src\PackageWriter.cpp' $baseline.packageIdentity.tocDomain 'package TOC domain'

$topologyPath = Join-Path $repositoryRoot 'engine_adapter\include\drs\engine\PublishedMacroBinding.h'
$topologyText = Get-Content -LiteralPath $topologyPath -Raw
foreach ($parameterId in $baseline.hostParameters) {
    if (-not $topologyText.Contains('"' + $parameterId + '"')) {
        $failures.Add("Host parameter ID changed or is missing: $parameterId")
    }
}

$vst3Root = Join-Path $repositoryRoot ("build\vs2022-debug\app\drs_plugin_bundle_artefacts\{0}\VST3" -f $BuildConfiguration)
$moduleInfoFiles = if (Test-Path -LiteralPath $vst3Root) {
    @(Get-ChildItem -LiteralPath $vst3Root -Filter moduleinfo.json -File -Recurse)
} else {
    @()
}
if ($moduleInfoFiles.Count -gt 0) {
    foreach ($moduleInfoFile in $moduleInfoFiles) {
        $moduleInfo = Get-Content -LiteralPath $moduleInfoFile.FullName -Raw
        foreach ($cid in @($baseline.pluginIdentity.componentCid, $baseline.pluginIdentity.controllerCid)) {
            if (-not $moduleInfo.Contains($cid)) {
                $failures.Add("Built VST3 module metadata is missing stable CID ${cid}: $($moduleInfoFile.FullName)")
            }
        }
    }
} else {
    Write-Warning "Built VST3 module metadata was not found; source identity checks still ran: $vst3Root"
}

if ($failures.Count -ne 0) {
    foreach ($failure in $failures) {
        Write-Error $failure
    }
    exit 1
}

Write-Host 'Technical identity baseline verification passed.'
Write-Host ("  project={0}" -f $baseline.cmakeIdentity.projectName)
Write-Host ("  plug-in codes={0}/{1}" -f $baseline.pluginIdentity.manufacturerCodeFourCc, $baseline.pluginIdentity.pluginCodeFourCc)
Write-Host ("  VST3 CIDs={0}, {1}" -f $baseline.pluginIdentity.componentCid, $baseline.pluginIdentity.controllerCid)
Write-Host ("  native formats={0}" -f ($baseline.nativeFormats -join ', '))
Write-Host ("  host parameters={0}" -f $baseline.hostParameters.Count)
