param(
    [string]$Compiler = $env:CC,
    [ValidateSet('all', 'read', 'init', 'timing', 'snapshot', 'health')]
    [string]$Group = 'all'
)
$ErrorActionPreference = 'Stop'

if (-not $Compiler) {
    $nativeCompiler = Get-Command gcc -ErrorAction SilentlyContinue
    if ($nativeCompiler) { $Compiler = $nativeCompiler.Source }
    else { throw 'Pass -Compiler with the path to a native GCC C compiler.' }
}

$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$build = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$exe = Join-Path $build 'sensor-tests.exe'
$env:PATH = (Split-Path $Compiler -Parent) + ';' + $env:PATH
& $Compiler -std=c99 -Wall -Wextra -I (Join-Path $PSScriptRoot 'mocks') `
    -I (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Inc') `
    (Join-Path $PSScriptRoot 'test_sensor.c') -o $exe -lm
if ($LASTEXITCODE -ne 0) { throw 'Sensor regression compilation failed.' }
& $exe $Group
if ($LASTEXITCODE -ne 0) { throw 'Sensor regressions failed.' }
