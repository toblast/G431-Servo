param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$compilerCommand = Get-Command $Compiler -ErrorAction Stop
$env:PATH = (Split-Path $compilerCommand.Source) + ';' + $env:PATH
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmwareRoot = Join-Path $repoRoot '01-Firmware/B-G431B-ESC1-v3'
$buildDirectory = Join-Path $repoRoot 'build/tests/cordic'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$source = Get-Content (Join-Path $firmwareRoot 'Core/Src/foc.c') -Raw
$globalStart = $source.IndexOf('// FOC state variable')
$globalEnd = $source.IndexOf('void API_FOC_Torque_Enable()', $globalStart)
$start = $source.IndexOf('void API_FOC_Torque_Update()' + "`r`n" + '{')
if ($start -lt 0) { $start = $source.IndexOf('void API_FOC_Torque_Update()' + "`n" + '{') }
$end = $source.IndexOf('uint32_t API_FOC_Get_Timestamp_ms()', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Missing production FOC update.' }
$fixture = Get-Content (Join-Path $PSScriptRoot 'cordic.c') -Raw
$generated = $fixture.Replace('/* GLOBALS */', $source.Substring($globalStart, $globalEnd - $globalStart)).Replace('/* UPDATE */', $source.Substring($start, $end - $start))
$sourcePath = Join-Path $buildDirectory 'cordic.c'
$exePath = Join-Path $buildDirectory 'cordic.exe'
[IO.File]::WriteAllText($sourcePath, $generated)
& $compilerCommand.Source -std=gnu99 -O2 -Wall -Wextra -Wno-unused-variable -I (Join-Path $PSScriptRoot 'mocks') -I (Join-Path $firmwareRoot 'Core/Inc') $sourcePath (Join-Path $firmwareRoot 'Core/Src/math_tool.c') -o $exePath -lm
if ($LASTEXITCODE -ne 0) { throw 'CORDIC compilation failed.' }
& $exePath
if ($LASTEXITCODE -ne 0) { throw 'CORDIC regression failed.' }
