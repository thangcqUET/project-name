<#
Generate an LVGL C font that includes Vietnamese glyphs using lv_font_conv.

Prerequisites:
- Node.js + npm installed
- lv_font_conv available via npm (we'll use npx to run it)

Usage (PowerShell):
  ./generate_font.ps1 -ttfPath "C:\path\to\NotoSans-Regular.ttf" -size 20 -outName myfont

This will create myfont.c and myfont.h in this folder.
#>

param(
    [Parameter(Mandatory=$true)] [string]$ttfPath,
    [int]$size = 20,
    [int]$bpp = 4,
    [string]$outName = "myfont"
)

Set-StrictMode -Version Latest

if (-not (Test-Path $ttfPath)) {
    Write-Error "TTF file not found: $ttfPath"
    exit 1
}

$outDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$outC = Join-Path -Path $outDir -ChildPath "$outName.c"
$outH = Join-Path -Path $outDir -ChildPath "$outName.h"

Write-Host "Generating LVGL font from: $ttfPath"
Write-Host "Output: $outC and $outH, size=$size, bpp=$bpp"

# Vietnamese ranges: basic Latin + Latin Extended-A + precomposed Vietnamese letters
$ranges = @(
    '0x20-0x7F',         # Basic Latin
    '0xA0-0xFF',         # Latin-1 Supplement
    '0x0100-0x017F',     # Latin Extended-A
    '0x0180-0x024F',     # Latin Extended-B (includes ơ, ư)
    '0x1EA0-0x1EFF'      # Vietnamese precomposed letters (wider range)
)

$args = @('lv_font_conv', '--font', $ttfPath)
foreach ($r in $ranges) { $args += @('-r', $r) }
$args += @('--size', $size.ToString(), '--bpp', $bpp.ToString(), '--format', 'lvgl', '--no-compress', '-o', $outC)

Write-Host "Running: npx $($args -join ' ')" -ForegroundColor Cyan

try {
    & npx @args
    if ($LASTEXITCODE -ne 0) { throw "lv_font_conv failed (exit $LASTEXITCODE)" }
}
catch {
    Write-Error "Font generation failed: $_"
    exit 1
}

Write-Host "Generated: $outC and $outH"
