param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$compilerCommand = Get-Command $Compiler -ErrorAction Stop
$env:PATH = (Split-Path $compilerCommand.Source) + ';' + $env:PATH
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmwareRoot = Join-Path $repoRoot '01-Firmware/B-G431B-ESC1-v3'
$buildDirectory = Join-Path $repoRoot 'build/tests/foc'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$source = Get-Content (Join-Path $firmwareRoot 'Core/Src/foc.c') -Raw
$functions = ''
foreach ($name in @('API_FOC_Torque_Enable', 'API_FOC_Torque_Disable', 'API_FOC_Set_Torque_Flux_Currents_mA')) {
    $start = $source.IndexOf("void $name(")
    $opening = $source.IndexOf('{', $start)
    $depth = 1
    for ($end = $opening + 1; $depth -gt 0 -and $end -lt $source.Length; $end++) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
    }
    if ($start -lt 0 -or $depth -ne 0) { throw "Missing production function: $name" }
    $functions += $source.Substring($start, $end - $start) + "`n"
}
$states = [regex]::Matches($source, '(?m)^#define FOC_STATE_.*$').Value -join "`n"
$globals = [regex]::Matches($source, '(?m)^static (?:volatile )?(?:uint32_t|float|pid_context_t|bool) (?:foc_state|setpoint_\w+|present_Ids_mA|present_Iqs_mA|flux_pi|torque_pi|calibration_active|calibration_cancelled)[^\r\n]*;').Value -join "`n"
$fixture = Get-Content (Join-Path $PSScriptRoot 'transitions.c') -Raw
$generated = $fixture.Replace('/* GLOBALS */', $states + "`n" + $globals).Replace('/* FUNCTIONS */', $functions)
$sourcePath = Join-Path $buildDirectory 'transitions.c'
$exePath = Join-Path $buildDirectory 'transitions.exe'
[IO.File]::WriteAllText($sourcePath, $generated)
& $compilerCommand.Source -std=gnu99 -Wall -Wextra -I (Join-Path $repoRoot 'tests/sensor/mocks') -I (Join-Path $firmwareRoot 'Core/Inc') $sourcePath -o $exePath
if ($LASTEXITCODE -ne 0) { throw 'FOC transition compilation failed.' }
& $exePath
if ($LASTEXITCODE -ne 0) { throw 'FOC transition regression failed.' }
$outputsExe = Join-Path $buildDirectory 'outputs.exe'
& $compilerCommand.Source -std=gnu99 -Wall -Wextra -I (Join-Path $PSScriptRoot 'mocks') -I (Join-Path $firmwareRoot 'Core/Inc') (Join-Path $PSScriptRoot 'outputs.c') (Join-Path $firmwareRoot 'Core/Src/math_tool.c') -o $outputsExe -lm
if ($LASTEXITCODE -ne 0) { throw 'FOC output compilation failed.' }
& $outputsExe
if ($LASTEXITCODE -ne 0) { throw 'FOC output regression failed.' }
