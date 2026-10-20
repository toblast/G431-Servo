param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = Join-Path $root 'build/tests/configuration'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$exe = Join-Path $build 'configuration.exe'
& $cc -std=gnu99 -Wall -Wextra -I (Join-Path $root 'tests/sensor/mocks') -I (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Inc') (Join-Path $PSScriptRoot 'configuration.c') -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Configuration regression compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Configuration regressions failed.' }
