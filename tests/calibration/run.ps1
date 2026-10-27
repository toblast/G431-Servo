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
foreach ($name in @('API_FOC_Is_Calibrating', 'API_FOC_Torque_Disable', 'API_FOC_Check_Watchdog', 'API_FOC_Calibration_Idle', 'API_FOC_Calibration_Begin', 'API_FOC_Calibration_SetFlux', 'API_FOC_Calibration_Cancelled', 'API_FOC_Calibration_End')) {
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
[IO.File]::WriteAllText($c, $generated)
foreach ($optimization in @('O0', 'O2')) {
    $exe = Join-Path $build "calibration-$optimization.exe"
    & $cc -std=gnu99 "-$optimization" -Wall -Wextra -Werror -I (Join-Path $root 'tests/sensor/mocks') -I (Join-Path $firmware 'Core/Inc') $c (Join-Path $firmware 'Core/Src/calibration.c') (Join-Path $firmware 'Core/Src/math_tool.c') (Join-Path $firmware 'Core/Src/drive_faults.c') (Join-Path $firmware 'Core/Src/motor_config.c') -o $exe -lm
    if ($LASTEXITCODE -ne 0) { throw "Calibration regression compilation failed ($optimization)." }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Calibration regressions failed ($optimization)." }
}
