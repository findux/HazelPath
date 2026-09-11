param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ tools not found.' }
$testDirectory = Join-Path $env:TEMP ('HazelPath-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDirectory | Out-Null
$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
$runner = Join-Path $testDirectory 'run.cmd'
@"
@echo off
call "$vcvars" >nul
cl /nologo /EHsc /std:c++17 /O2 /Isrc tests\simulation_visibility_test.cpp src\simulation\SimulationController.cpp src\simulation\MotionInterpolator.cpp src\toolpath\Toolpath.cpp /Fo"$testDirectory\\" /Fe"$testDirectory\simulation_test.exe"
if errorlevel 1 exit /b 1
"$testDirectory\simulation_test.exe"
"@ | Set-Content -LiteralPath $runner -Encoding ASCII
Push-Location $projectRoot
try {
    & $env:ComSpec /c $runner
    if ($LASTEXITCODE -ne 0) { throw "Simulation tests failed: $LASTEXITCODE" }
} finally {
    Pop-Location
}
