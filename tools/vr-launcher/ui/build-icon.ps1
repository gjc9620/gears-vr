param([string]$Source, [string]$Destination)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not $Source) { $Source = Join-Path $PSScriptRoot 'assets/launcher-artwork.png' }
if (-not $Destination) { $Destination = Join-Path $PSScriptRoot 'assets/launcher.ico' }
$sourceImage = [Drawing.Image]::FromFile([IO.Path]::GetFullPath($Source))
$frames = @()
try {
    foreach ($size in @(16, 24, 32, 48, 64, 128, 256)) {
        $bitmap = New-Object Drawing.Bitmap($size, $size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        $png = New-Object IO.MemoryStream
        try {
            $graphics.Clear([Drawing.Color]::Transparent)
            $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
            $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $padding = [Math]::Max(1, [int][Math]::Round($size * 0.03))
            $rectangle = New-Object Drawing.Rectangle($padding, $padding, ($size - 2 * $padding), ($size - 2 * $padding))
            $graphics.DrawImage($sourceImage, $rectangle)
            $bitmap.Save($png, [Drawing.Imaging.ImageFormat]::Png)
            $frames += [pscustomobject]@{ Size = $size; Bytes = $png.ToArray() }
        } finally { $graphics.Dispose(); $bitmap.Dispose(); $png.Dispose() }
    }
} finally { $sourceImage.Dispose() }
$output = New-Object IO.MemoryStream
$writer = New-Object IO.BinaryWriter($output)
try {
    $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]$frames.Count)
    $offset = 6 + 16 * $frames.Count
    foreach ($frame in $frames) {
        $dimension = if ($frame.Size -eq 256) { 0 } else { $frame.Size }
        $writer.Write([byte]$dimension); $writer.Write([byte]$dimension)
        $writer.Write([byte]0); $writer.Write([byte]0)
        $writer.Write([uint16]1); $writer.Write([uint16]32)
        $writer.Write([uint32]$frame.Bytes.Length); $writer.Write([uint32]$offset)
        $offset += $frame.Bytes.Length
    }
    foreach ($frame in $frames) { $writer.Write([byte[]]$frame.Bytes) }
    $writer.Flush()
    [IO.File]::WriteAllBytes([IO.Path]::GetFullPath($Destination), $output.ToArray())
    $mark = @($frames | Where-Object { $_.Size -eq 32 })[0]
    [IO.File]::WriteAllBytes([IO.Path]::ChangeExtension([IO.Path]::GetFullPath($Destination), '.png'), [byte[]]$mark.Bytes)
} finally { $writer.Dispose(); $output.Dispose() }
Write-Output "Icon: $Destination (16/24/32/48/64/128/256 px)"
