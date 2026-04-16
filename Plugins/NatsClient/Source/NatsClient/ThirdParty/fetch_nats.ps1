# fetch_nats.ps1 — robust cross-platform vendor script for nats.c

param(
    [switch]$clean,
    [string[]]$platform
)

$ErrorActionPreference = "Stop"

$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$NATS_DIR   = Join-Path $SCRIPT_DIR "nats.c"
$SRC_DIR    = Join-Path $NATS_DIR "src"
$NATS_VERSION = if ($env:NATS_VERSION) { $env:NATS_VERSION } else { "v3.8.2" }

# -------------------------
# Helpers
# -------------------------

function Require-Cmd($cmd) {
    if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) {
        throw "Missing required command: $cmd"
    }
}

function Normalize-Platform($p) {
    switch ($p.ToLower()) {
        "windows" { "Windows" }
        "win"     { "Windows" }
        "linux"   { "Linux" }
        "mac"     { "Mac" }
        "macos"   { "Mac" }
        "darwin"  { "Mac" }
        default   { "" }
    }
}

function Detect-Default-Platform {
    if ($IsWindows) { return "Win64" }
    if ($IsLinux)   { return "Linux" }
    if ($IsMacOS)   { return "Mac" }
    return ""
}

function Find-BuiltLib($buildDir, $platform) {
    if ($platform -eq "Win64") {
        $libs = Get-ChildItem -Recurse -Path $buildDir -Filter "*.lib" -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -match "nats" }

        if (-not $libs) { return $null }

        # Prefer Release
        $preferred = $libs | Where-Object { $_.FullName -match "Release" } | Select-Object -First 1
        if ($preferred) { return $preferred.FullName }

        return ($libs | Select-Object -First 1).FullName
    }
    else {
        $libs = Get-ChildItem -Recurse -Path $buildDir -Filter "*.a" -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -match "nats" }

        if (-not $libs) { return $null }

        return ($libs | Select-Object -First 1).FullName
    }
}

function Build-For($platform) {
    $buildDir = Join-Path $NATS_DIR "build/$platform"
    $libDir   = Join-Path $NATS_DIR "lib/$platform"

    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    New-Item -ItemType Directory -Force -Path $libDir   | Out-Null

    Write-Host "==> Building for $platform..."

    if ($platform -eq "Windows") {
        # Force MSVC generator for consistency
        cmake -S $SRC_DIR -B $buildDir `
            -G "Visual Studio 17 2022" -A x64 `
            -DNATS_BUILD_LIB_STATIC=ON `
            -DNATS_BUILD_LIB_SHARED=OFF `
            -DNATS_BUILD_EXAMPLES=OFF `
            -DBUILD_TESTING=OFF `
            -DNATS_BUILD_STREAMING=OFF `
            -DNATS_BUILD_WITH_TLS=OFF

        # Try both possible targets (nats_static OR nats)
        cmake --build $buildDir --config Release --target nats_static 2>$null
        if ($LASTEXITCODE -ne 0) {
            cmake --build $buildDir --config Release --target nats
        }
    }
    else {
        cmake -S $SRC_DIR -B $buildDir `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON `
            -DCMAKE_C_FLAGS="-fPIC" `
            -DCMAKE_CXX_FLAGS="-fPIC" `
            -DNATS_BUILD_LIB_STATIC=ON `
            -DNATS_BUILD_LIB_SHARED=OFF `
            -DNATS_BUILD_EXAMPLES=OFF `
            -DBUILD_TESTING=OFF `
            -DNATS_BUILD_STREAMING=OFF `
            -DNATS_BUILD_WITH_TLS=OFF

        cmake --build $buildDir --target nats_static 2>$null
        if ($LASTEXITCODE -ne 0) {
            cmake --build $buildDir --target nats
        }
    }

    $lib = Find-BuiltLib $buildDir $platform

    if (-not $lib) {
        Write-Host ""
        Write-Host "❌ Build succeeded but library not found."
        Write-Host "Contents of build dir:"
        Get-ChildItem -Recurse $buildDir | Select-Object FullName
        throw "Failed to locate built nats library"
    }

    $ext = [System.IO.Path]::GetExtension($lib)
    $dest = Join-Path $libDir ("nats_static" + $ext)

    Copy-Item $lib $dest -Force
    Write-Host "   -> $dest"
}

# -------------------------
# Start
# -------------------------

Require-Cmd git
Require-Cmd cmake

# Platforms
$PLATFORMS = @()

if ($platform) {
    foreach ($p in $platform) {
        $n = Normalize-Platform $p
        if (-not $n) { throw "Unsupported platform: $p" }
        $PLATFORMS += $n
    }
}
else {
    $detected = Detect-Default-Platform
    if (-not $detected) {
        throw "Could not detect platform"
    }
    $PLATFORMS = @($detected)
}

# Clean
if ($clean) {
    Write-Host "==> Cleaning..."
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
        (Join-Path $NATS_DIR "build"),
        (Join-Path $NATS_DIR "include"),
        (Join-Path $NATS_DIR "lib")
}

# Clone
Write-Host "==> Cloning nats.c $NATS_VERSION..."
if (-not (Test-Path $SRC_DIR)) {
    git clone --depth 1 --branch $NATS_VERSION https://github.com/nats-io/nats.c.git $SRC_DIR
}

# Headers
Write-Host "==> Copying headers..."

$includeBase = Join-Path $NATS_DIR "include"
$includeNats = Join-Path $includeBase "nats"
$includeAdapters = Join-Path $includeNats "adapters"

New-Item -ItemType Directory -Force -Path $includeAdapters | Out-Null

$hdrSrc = if (Test-Path "$SRC_DIR/src/nats.h") {
    "$SRC_DIR/src"
} else {
    "$SRC_DIR"
}

Copy-Item "$hdrSrc/nats.h"    "$includeBase/nats.h" -Force
Copy-Item "$hdrSrc/nats.h"    "$includeNats/nats.h" -Force
Copy-Item "$hdrSrc/status.h"  "$includeNats/status.h" -Force
Copy-Item "$hdrSrc/version.h" "$includeNats/version.h" -Force

if (Test-Path "$hdrSrc/adapters") {
    Copy-Item "$hdrSrc/adapters/*.h" $includeAdapters -ErrorAction SilentlyContinue
}

# Build
foreach ($p in $PLATFORMS) {
    Build-For $p
}

# Cleanup
Write-Host "==> Pruning sources..."
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue `
    (Join-Path $NATS_DIR "src"),
    (Join-Path $NATS_DIR "build")

Write-Host ""
Write-Host "✅ Done!"
Write-Host "Output:"
Write-Host "  $NATS_DIR/include"
Write-Host "  $NATS_DIR/lib/<Platform>"