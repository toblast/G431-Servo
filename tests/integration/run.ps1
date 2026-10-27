param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmware = Join-Path $root '01-Firmware/B-G431B-ESC1-v3'
$build = Join-Path $root 'build/tests/integration'
New-Item -ItemType Directory -Force -Path $build | Out-Null

# Keep the real FOC permission/cancellation/watchdog boundary; ADC/PWM/math
# execution remains covered by the existing FOC/CORDIC suites, not simulated here.
$source = Get-Content (Join-Path $firmware 'Core/Src/foc.c') -Raw
$functions = ''
foreach ($name in @('API_FOC_Torque_Enable', 'API_FOC_Torque_Disable',
    'API_FOC_Set_Torque_Flux_Currents_mA', 'API_FOC_Is_Calibrating',
    'API_FOC_Check_Watchdog', 'API_FOC_Calibration_Idle',
    'API_FOC_Calibration_Begin', 'API_FOC_Calibration_SetFlux',
    'API_FOC_Calibration_Cancelled', 'API_FOC_Calibration_End')) {
    $match = [regex]::Match($source, "(?m)^(?:void|bool) $name\(")
    if (!$match.Success) { throw "Missing FOC boundary function: $name" }
    $start = $match.Index
    $opening = $source.IndexOf('{', $start)
    $depth = 1
    for ($end = $opening + 1; $depth -gt 0 -and $end -lt $source.Length; $end++) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
    }
    if ($depth -ne 0) { throw "Unclosed FOC function: $name" }
    $functions += $source.Substring($start, $end - $start) + "`n"
}
$states = [regex]::Matches($source, '(?m)^#define FOC_STATE_.*$').Value -join "`n"
$globals = [regex]::Matches($source, '(?m)^static (?:volatile )?(?:uint32_t|float|bool|pid_context_t) (?:foc_state|setpoint_\w+|calibration_\w+|present_I[qd]s_mA|flux_pi|torque_pi)[^\r\n]*;').Value -join "`n"
$fixture = Get-Content (Join-Path $PSScriptRoot 'integration.c') -Raw
$generated = $fixture.Replace('/* FOC_GLOBALS */', $states + "`n" + $globals).Replace('/* FOC_FUNCTIONS */', $functions)
$c = Join-Path $build 'integration.c'
[IO.File]::WriteAllText($c, $generated)

$modules = @('app', 'drive', 'servo_controller', 'can_servo', 'calibration',
    'position_sensor', 'as5600', 'configuration', 'config_validation',
    'config_storage', 'control_table', 'motor_config', 'drive_faults',
    'protocol', 'pic', 'math_tool') | ForEach-Object { Join-Path $firmware "Core/Src/$_.c" }
foreach ($optimization in @('O0', 'O2')) {
    $exe = Join-Path $build "integration-$optimization.exe"
    & $cc -std=gnu99 -funsigned-char "-$optimization" -Wall -Wextra -Werror `
        -I (Join-Path $PSScriptRoot 'stubs') -I (Join-Path $firmware 'Core/Inc') `
        $c @modules -o $exe -lm
    if ($LASTEXITCODE -ne 0) { throw "Integration compilation failed ($optimization)." }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Integration regressions failed ($optimization)." }
}
