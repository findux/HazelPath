param([string]$NcFile)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$luaSource = [IO.Path]::GetFullPath((Join-Path $projectRoot '..\..\..\addons\ofxLua\libs\lua\src'))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ tools not found.' }
$testDirectory = Join-Path $env:TEMP ('PCNI-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDirectory | Out-Null
$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
$sources = (Get-ChildItem -LiteralPath $luaSource -Filter '*.c' | Where-Object Name -ne 'luac.c' | ForEach-Object { '"' + $_.FullName + '"' }) -join ' '
$ncArgument = if ($NcFile) { ' "' + (Resolve-Path -LiteralPath $NcFile).Path + '"' } else { '' }
$runner = Join-Path $testDirectory 'run.cmd'
@"
@echo off
call "$vcvars" >nul
cl /nologo /O2 /DLUA_USE_WINDOWS $sources /Fo"$testDirectory\\" /Fe"$testDirectory\lua.exe" >"$testDirectory\build.log" 2>&1
if errorlevel 1 type "$testDirectory\build.log"
if errorlevel 1 exit /b 1
"$testDirectory\lua.exe" tests\pcni_import_test.lua$ncArgument
"@ | Set-Content -LiteralPath $runner -Encoding ASCII
Push-Location $projectRoot
try {
    & $env:ComSpec /c $runner
    if ($LASTEXITCODE -ne 0) { throw "PCNI tests failed: $LASTEXITCODE" }
} finally { Pop-Location }
