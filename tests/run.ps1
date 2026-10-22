param([string]$Compiler = $env:CC)
$ErrorActionPreference = 'Stop'
if (!$Compiler) {
    $found = Get-Command gcc -ErrorAction SilentlyContinue
    if (!$found) { throw 'Pass -Compiler with a native GCC executable.' }
    $Compiler = $found.Source
}
foreach ($suite in @('startup', 'sensor', 'can', 'foc', 'calibration', 'protocol', 'serial', 'telemetry', 'configuration', 'fatal', 'cordic')) {
    & (Join-Path $PSScriptRoot "$suite/run.ps1") -Compiler $Compiler
    if ($LASTEXITCODE -ne 0) { throw "Regression suite failed: $suite" }
}
Write-Output 'All eleven regression suites passed.'
