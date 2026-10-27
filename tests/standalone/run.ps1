param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmware = Join-Path $root '01-Firmware/B-G431B-ESC1-v3'
$build = Join-Path $root 'build/tests/standalone'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$sources = @((Join-Path $PSScriptRoot 'standalone.c'))
foreach ($name in @('cruise_control', 'vehicle_inputs', 'standalone', 'app',
    'drive', 'servo_controller', 'can_servo', 'drive_faults', 'control_table',
    'config_validation', 'pic', 'math_tool')) {
    $sources += Join-Path $firmware "Core/Src/$name.c"
}
foreach ($optimization in @('O0', 'O2')) {
  foreach ($reverse in @(0, 1)) {
    $exe = Join-Path $build "standalone-$optimization-$reverse.exe"
    & $cc -std=gnu99 "-$optimization" -DDRIVE_STANDALONE=1 "-DTEST_REVERSE_TRAVEL=$reverse" -Wall -Wextra -Werror -I (Join-Path $PSScriptRoot 'stubs') -I (Join-Path $firmware 'Core/Inc') @sources -o $exe -lm
    if ($LASTEXITCODE -ne 0) { throw "Standalone compilation failed ($optimization)." }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Standalone regressions failed ($optimization)." }
  }
}
