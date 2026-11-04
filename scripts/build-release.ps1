param(
    [Parameter(Mandatory)][string]$Tag,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$MakeNSIS = 'C:\Program Files (x86)\NSIS\makensis.exe'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$versionHeader = Get-Content -Raw (Join-Path $projectRoot 'src/AppVersion.h')
if ($versionHeader -notmatch '#define\s+HAZELPATH_VERSION\s+"(\d+\.\d+\.\d+)"') { throw 'Invalid AppVersion.h.' }
$version = $Matches[1]
if ($Tag -cne "v$version") { throw "Tag $Tag does not match AppVersion.h (v$version)." }
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Use a fresh output directory.' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
& (Join-Path $projectRoot 'tests/run_simulation_visibility_tests.ps1')
& (Join-Path $projectRoot 'tests/run_pcni_import_tests.ps1')
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsInstallation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$msbuild = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'Visual Studio C++ MSBuild not found.' }
# Ship the CRT matching the build tools, rather than old DLLs from a past release.
$redistVersion = (Get-Content -Raw (Join-Path $vsInstallation 'VC/Auxiliary/Build/Microsoft.VCRedistVersion.default.txt')).Trim()
$redistRoot = Join-Path $vsInstallation "VC/Redist/MSVC/$redistVersion/x64/Microsoft.VC143.CRT"
foreach ($runtimeDll in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    Copy-Item -LiteralPath (Join-Path $redistRoot $runtimeDll) -Destination (Join-Path $projectRoot "installer/runtime/$runtimeDll")
}
Push-Location $projectRoot
try {
    & $msbuild NC_kod_duzeltici.sln /m:2 /t:Rebuild /p:Configuration=Release /p:Platform=x64 /verbosity:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed: $LASTEXITCODE" }
    if (-not (Test-Path 'bin/HazelPath.exe')) { throw 'Build did not produce HazelPath.exe.' }
    New-Item -ItemType Directory -Force dist | Out-Null
    & $MakeNSIS /DAPP_BUILD_DATE=ci installer/HazelPath.nsi
    if ($LASTEXITCODE -ne 0) { throw "NSIS failed: $LASTEXITCODE" }
    Copy-Item -LiteralPath "dist/HazelPath-Setup-$version-ci-x64.exe" -Destination (Join-Path $OutputDirectory "HazelPath-Setup-$version-x64.exe")
} finally { Pop-Location }
