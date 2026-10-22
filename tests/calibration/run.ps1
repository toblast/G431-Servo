param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmware = Join-Path $root '01-Firmware/B-G431B-ESC1-v3'
$build = Join-Path $root 'build/tests/calibration'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$source = Get-Content (Join-Path $firmware 'Core/Src/foc.c') -Raw
$functions = ''
foreach ($name in @('API_FOC_Is_Calibrating', 'API_FOC_Torque_Disable', 'API_FOC_Check_Watchdog', 'FOC_Calibration_Idle', 'FOC_Calibration_Wait', 'FOC_Calibration_Enter_Flux', 'API_FOC_Calibrate')) {
    $match = [regex]::Match($source, "(?m)^(?:static )?(?:void|bool|int) $name\(")
    if (!$match.Success) { throw "Missing calibration function: $name" }
    $start = $match.Index
    $opening = $source.IndexOf('{', $start)
    $depth = 1
    for ($end = $opening + 1; $depth -gt 0 -and $end -lt $source.Length; $end++) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
    }
    $functions += $source.Substring($start, $end - $start) + "`n"
}
$states = [regex]::Matches($source, '(?m)^#define FOC_STATE_.*$').Value -join "`n"
$globals = [regex]::Matches($source, '(?m)^static (?:volatile )?(?:uint32_t|uint16_t|float|bool) (?:foc_state|setpoint_\w+|calibration_\w+)[^\r\n]*;').Value -join "`n"
$fixture = Get-Content (Join-Path $PSScriptRoot 'calibration.c') -Raw
$generated = $fixture.Replace('/* GLOBALS */', $states + "`n" + $globals).Replace('/* FUNCTIONS */', $functions)
$c = Join-Path $build 'calibration.c'
$exe = Join-Path $build 'calibration.exe'
[IO.File]::WriteAllText($c, $generated)
& $cc -std=gnu99 -Wall -Wextra -I (Join-Path $root 'tests/sensor/mocks') -I (Join-Path $firmware 'Core/Inc') $c (Join-Path $firmware 'Core/Src/math_tool.c') -o $exe -lm
if ($LASTEXITCODE -ne 0) { throw 'Calibration regression compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Calibration regressions failed.' }
