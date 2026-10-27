param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = Join-Path $root 'build/tests/protocol'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$exe = Join-Path $build 'protocol.exe'
& $cc -std=gnu99 -funsigned-char -Wall -Wextra -I (Join-Path $PSScriptRoot 'mocks') -I (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Inc') (Join-Path $PSScriptRoot 'protocol.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c') -o $exe
if ($LASTEXITCODE -ne 0) { throw 'Serial protocol regression compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Serial protocol regressions failed.' }
