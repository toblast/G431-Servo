param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = Join-Path $root 'build/tests/serial'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$exe = Join-Path $build 'serial.exe'
& $cc -std=gnu99 -Wall -Wextra -Werror -I (Join-Path $PSScriptRoot 'mocks') -I (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Inc') (Join-Path $PSScriptRoot 'serial.c') -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Serial transport compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Serial transport regressions failed.' }
