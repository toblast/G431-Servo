param(
    [string]$Compiler = 'gcc',
    [string]$FirmwareRoot = (Join-Path $PSScriptRoot '../../01-Firmware/B-G431B-ESC1-v3')
)
$ErrorActionPreference = 'Stop'
$compilerCommand = Get-Command $Compiler -ErrorAction Stop
$env:PATH = (Split-Path $compilerCommand.Source) + ';' + $env:PATH
$buildDirectory = Join-Path $PSScriptRoot '../../build/tests/startup'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

function Get-FunctionBody([string]$Source, [string]$Signature) {
    $start = $Source.IndexOf($Signature)
    if ($start -lt 0) { throw "Missing function: $Signature" }
    $opening = $Source.IndexOf('{', $start)
    $depth = 1
    for ($end = $opening + 1; $depth -gt 0 -and $end -lt $Source.Length; $end++) {
        if ($Source[$end] -eq '{') { $depth++ }
        if ($Source[$end] -eq '}') { $depth-- }
    }
    if ($depth -ne 0) { throw "Unclosed function: $Signature" }
    return $Source.Substring($start, $end - $start)
}

$mainSource = Get-Content (Join-Path $FirmwareRoot 'Core/Src/main.c') -Raw
$focSource = Get-Content (Join-Path $FirmwareRoot 'Core/Src/foc.c') -Raw
$mainStart = $mainSource.IndexOf('int main(void)')
$startupStart = $mainSource.IndexOf('/* USER CODE BEGIN 2 */', $mainStart)
$startupEnd = $mainSource.IndexOf('/* USER CODE END 2 */', $startupStart)
$startup = $mainSource.Substring($startupStart, $startupEnd - $startupStart)
$focInit = Get-FunctionBody $focSource 'void API_FOC_Init()'
$fixture = Get-Content (Join-Path $PSScriptRoot 'startup_fixture.c') -Raw
$source = $fixture.Replace('/* FOC_INIT */', $focInit).Replace('/* STARTUP */', $startup)
$buttonState = [regex]::Match($mainSource, 'bool calibration_button_was_down = [^;]+;').Value
$buttonEdge = [regex]::Match($mainSource, '(?s)bool const calibration_button_down = .*?calibration_button_was_down = calibration_button_down;').Value
if (!$buttonState -or !$buttonEdge) { throw 'Missing production calibration button logic.' }
$source = $source.Replace('/* BUTTON_STATE */', $buttonState).Replace('/* BUTTON_EDGE */', $buttonEdge)
$sourcePath = Join-Path $buildDirectory 'startup_tests.c'
$executablePath = Join-Path $buildDirectory 'startup_tests.exe'
[IO.File]::WriteAllText($sourcePath, $source)
& $compilerCommand.Source -std=gnu99 -Wall -Wextra $sourcePath -o $executablePath
if ($LASTEXITCODE -ne 0) { throw 'Startup regression compilation failed.' }
& $executablePath
if ($LASTEXITCODE -ne 0) { throw 'Startup regression failed.' }
