param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmware = Join-Path $root '01-Firmware/B-G431B-ESC1-v3'
$build = Join-Path $root 'build/tests/telemetry'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$exe = Join-Path $build 'telemetry.exe'
& $cc -std=gnu99 -O2 -Wall -Wextra -Werror -I (Join-Path $root 'tests/app/stubs') -I (Join-Path $root 'tests/app') -I (Join-Path $firmware 'Core/Inc') (Join-Path $PSScriptRoot 'telemetry.c') (Join-Path $root 'tests/app/app_fixture.c') (Join-Path $firmware 'Core/Src/app.c') (Join-Path $firmware 'Core/Src/control_table.c') (Join-Path $firmware 'Core/Src/drive_faults.c') (Join-Path $firmware 'Core/Src/config_validation.c') -o $exe -lm
if ($LASTEXITCODE -ne 0) { throw 'Telemetry compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Telemetry regressions failed.' }
