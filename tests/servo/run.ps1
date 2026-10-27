param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmware = Join-Path $root '01-Firmware/B-G431B-ESC1-v3'
$build = Join-Path $root 'build/tests/servo'
New-Item -ItemType Directory -Force -Path $build | Out-Null

# Link real translation units; do not extract or copy controller implementation.
$sources = @((Join-Path $PSScriptRoot 'servo.c'))
foreach ($name in @('servo_controller', 'drive', 'drive_faults', 'config_validation', 'control_table', 'pic', 'math_tool')) {
    $sources += Join-Path $firmware "Core/Src/$name.c"
}
foreach ($optimization in @('O0', 'O2')) {
    $exe = Join-Path $build "servo-$optimization.exe"
    $option = "-$optimization"
    & $cc -std=gnu99 $option -Wall -Wextra -Werror -I (Join-Path $root 'tests/can/stubs') -I (Join-Path $firmware 'Core/Inc') @sources -o $exe -lm
    if ($LASTEXITCODE -ne 0) { throw "Servo compilation failed ($option)." }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Servo regressions failed ($option)." }
}
