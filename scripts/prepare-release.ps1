param(
    [Parameter(Mandatory)][string]$SdkArchive,
    [Parameter(Mandatory)][string]$Destination
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$lock = Get-Content -Raw (Join-Path $projectRoot 'scripts/release-dependencies.json') | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $SdkArchive -Algorithm SHA256).Hash -ne $lock.openFrameworks.sha256) {
    throw 'openFrameworks SDK checksum mismatch.'
}
if (Test-Path -LiteralPath $Destination) { throw 'Use a new, empty SDK destination.' }
New-Item -ItemType Directory -Path $Destination | Out-Null
& 'C:\Program Files\7-Zip\7z.exe' x $SdkArchive "-o$Destination" -y | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'SDK extraction failed.' }
$sdkProjects = @(Get-ChildItem -LiteralPath $Destination -Recurse -Filter openframeworksLib.vcxproj | Where-Object { $_.Directory.Name -eq 'vs' })
if ($sdkProjects.Count -ne 1) { throw 'Expected exactly one openFrameworks SDK.' }
$sdkRoot = $sdkProjects[0].Directory.Parent.Parent.Parent.Parent.FullName
$appRoot = Join-Path $sdkRoot 'apps/myApps/HazelPath'
New-Item -ItemType Directory -Force -Path $appRoot | Out-Null
foreach ($directory in @('src', 'scripts', 'tests', 'res', 'installer', 'licenses', 'docs')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $directory) -Destination $appRoot -Recurse
}
New-Item -ItemType Directory -Force -Path (Join-Path $appRoot 'bin') | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'bin/data') -Destination (Join-Path $appRoot 'bin') -Recurse
foreach ($file in @('NC_kod_duzeltici.sln', 'NC_kod_duzeltici.vcxproj',
    'NC_kod_duzeltici.vcxproj.filters', 'addons.make', 'config.make',
    'icon.rc', 'icon.ico', 'icon_debug.ico', 'LICENSE', 'README.md')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $appRoot
}
foreach ($addon in Get-ChildItem -LiteralPath (Join-Path $projectRoot 'vendor/addons') -Directory) {
    if (Test-Path (Join-Path $sdkRoot "addons/$($addon.Name)")) {
        throw "SDK already contains addon $($addon.Name); refusing to merge snapshots."
    }
    Copy-Item -LiteralPath $addon.FullName -Destination (Join-Path $sdkRoot 'addons') -Recurse
}
# Preserve third-party license documents in the installer, retaining their paths.
$noticeRoot = Join-Path $appRoot 'licenses/dependencies'
foreach ($root in @((Join-Path $sdkRoot 'libs'), (Join-Path $projectRoot 'vendor/addons'))) {
    $prefix = if ((Split-Path $root -Leaf) -eq 'addons') { 'addons' } else { 'openFrameworks-libs' }
    foreach ($notice in Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object {
        $_.Name -match '(?i)license|copying|copyright|notice|readme-lua'
    }) {
        $target = Join-Path $noticeRoot "$prefix/$($notice.FullName.Substring($root.Length + 1))"
        New-Item -ItemType Directory -Force -Path (Split-Path $target -Parent) | Out-Null
        Copy-Item -LiteralPath $notice.FullName -Destination $target
    }
}
Copy-Item -LiteralPath (Join-Path $sdkRoot 'LICENSE.md') -Destination (Join-Path $appRoot 'licenses/openFrameworks.txt')
if ($env:GITHUB_OUTPUT) {
    "sdk_root=$sdkRoot" | Out-File -FilePath $env:GITHUB_OUTPUT -Encoding utf8 -Append
    "app_root=$appRoot" | Out-File -FilePath $env:GITHUB_OUTPUT -Encoding utf8 -Append
}
Write-Output $appRoot
