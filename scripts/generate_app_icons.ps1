
$ErrorActionPreference = "Stop"

$projectPath = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$sourcePath = Join-Path $projectPath "res\appicon.png"
$releaseIconPath = Join-Path $projectPath "icon.ico"
$debugIconPath = Join-Path $projectPath "icon_debug.ico"
if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
    throw "Application icon source was not found: $sourcePath"
}

function Test-FileBytesEqual {
    param(
        [string]$Path,
        [byte[]]$ExpectedBytes
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $false
    }
    $currentBytes = [System.IO.File]::ReadAllBytes($Path)
    if ($currentBytes.Length -ne $ExpectedBytes.Length) {
        return $false
    }
    for ($index = 0; $index -lt $currentBytes.Length; ++$index) {
        if ($currentBytes[$index] -ne $ExpectedBytes[$index]) {
            return $false
        }
    }
    return $true
}

Add-Type -AssemblyName System.Drawing
$source = [System.Drawing.Image]::FromFile($sourcePath)
try {
    if ($source.Width -ne $source.Height) {
        throw "Application icon must be square. Current size: $($source.Width)x$($source.Height)"
    }

    $sizes = @(16, 24, 32, 48, 64, 128, 256)
    $images = New-Object System.Collections.Generic.List[object]
    foreach ($size in $sizes) {
        $bitmap = New-Object System.Drawing.Bitmap(
            $size,
            $size,
            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.CompositingQuality =
                    [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode =
                    [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode =
                    [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.SmoothingMode =
                    [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.DrawImage($source, 0, 0, $size, $size)
            } finally {
                $graphics.Dispose()
            }

            $memory = New-Object System.IO.MemoryStream
            try {
                $bitmap.Save($memory, [System.Drawing.Imaging.ImageFormat]::Png)
                $images.Add([pscustomobject]@{
                    Size = $size
                    Bytes = $memory.ToArray()
                })
            } finally {
                $memory.Dispose()
            }
        } finally {
            $bitmap.Dispose()
        }
    }

    $output = New-Object System.IO.MemoryStream
    $writer = New-Object System.IO.BinaryWriter($output)
    try {
        $writer.Write([uint16]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]$images.Count)

        $offset = 6 + 16 * $images.Count
        foreach ($entry in $images) {
            $dimension = if ($entry.Size -eq 256) { 0 } else { $entry.Size }
            $writer.Write([byte]$dimension)
            $writer.Write([byte]$dimension)
            $writer.Write([byte]0)
            $writer.Write([byte]0)
            $writer.Write([uint16]1)
            $writer.Write([uint16]32)
            $writer.Write([uint32]$entry.Bytes.Length)
            $writer.Write([uint32]$offset)
            $offset += $entry.Bytes.Length
        }

        foreach ($entry in $images) {
            $writer.Write([byte[]]$entry.Bytes)
        }
        $writer.Flush()
        $iconBytes = $output.ToArray()
    } finally {
        $writer.Dispose()
        $output.Dispose()
    }

    $releaseMatches = Test-FileBytesEqual $releaseIconPath $iconBytes
    $debugMatches = Test-FileBytesEqual $debugIconPath $iconBytes
    if (-not $releaseMatches) {
        [System.IO.File]::WriteAllBytes($releaseIconPath, $iconBytes)
    }
    if (-not $debugMatches) {
        [System.IO.File]::WriteAllBytes($debugIconPath, $iconBytes)
    }
    if ($releaseMatches -and $debugMatches) {
        Write-Host "HazelPath application icons are up to date."
    } else {
        Write-Host "Generated HazelPath application icons from res\appicon.png."
    }
} finally {
    $source.Dispose()
}