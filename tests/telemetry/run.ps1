param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmware = Join-Path $root '01-Firmware/B-G431B-ESC1-v3'
$build = Join-Path $root 'build/tests/telemetry'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$source = [IO.File]::ReadAllText((Join-Path $firmware 'Core/Src/main.c'))
$update = [regex]::Match($source, '(?s)// RAM Update.*?(?=\r?\n\s*\} // low priority process 4Khz)').Value
if (!$update) { throw 'Production telemetry block not found.' }
$fixture = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'telemetry.c'))
$generated = Join-Path $build 'telemetry.c'
[IO.File]::WriteAllText($generated, $fixture.Replace('/* UPDATE */', $update))
$exe = Join-Path $build 'telemetry.exe'
& $cc -std=gnu99 -O2 -Wall -Wextra -Werror -I (Join-Path $root 'tests/sensor/mocks') -I (Join-Path $firmware 'Core/Inc') $generated (Join-Path $firmware 'Core/Src/control_table.c') -o $exe -lm
if ($LASTEXITCODE -ne 0) { throw 'Telemetry compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Telemetry regressions failed.' }
