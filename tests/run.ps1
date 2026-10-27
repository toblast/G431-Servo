param([string]$Compiler = $env:CC)
$ErrorActionPreference = 'Stop'
if (!$Compiler) {
    $found = Get-Command gcc -ErrorAction SilentlyContinue
    if (!$found) { throw 'Pass -Compiler with a native GCC executable.' }
    $Compiler = $found.Source
}
$suites = @('startup', 'sensor', 'can', 'servo', 'app', 'foc', 'calibration', 'protocol', 'serial', 'telemetry', 'configuration', 'fatal', 'cordic', 'integration')
foreach ($suite in $suites) {
    & (Join-Path $PSScriptRoot "$suite/run.ps1") -Compiler $Compiler
    if ($LASTEXITCODE -ne 0) { throw "Regression suite failed: $suite" }
}
Write-Output "All $($suites.Count) regression suites passed."
