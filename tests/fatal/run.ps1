param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$compilerCommand = Get-Command $Compiler -ErrorAction Stop
$env:PATH = (Split-Path $compilerCommand.Source) + ';' + $env:PATH
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmwareRoot = Join-Path $repoRoot '01-Firmware/B-G431B-ESC1-v3'
$buildDirectory = Join-Path $repoRoot 'build/tests/fatal'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$source = (Get-Content (Join-Path $firmwareRoot 'Core/Src/main.c') -Raw) + (Get-Content (Join-Path $firmwareRoot 'Core/Src/stm32g4xx_it.c') -Raw)
$functions = ''
foreach ($name in @('Error_Handler', 'NMI_Handler', 'HardFault_Handler', 'MemManage_Handler', 'BusFault_Handler', 'UsageFault_Handler')) {
    $start = $source.IndexOf("void $name(void)")
    $opening = $source.IndexOf('{', $start)
    $depth = 1
    for ($end = $opening + 1; $depth -gt 0 -and $end -lt $source.Length; $end++) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
    }
    if ($start -lt 0 -or $depth -ne 0) { throw "Missing handler: $name" }
    $functions += $source.Substring($start, $end - $start).Replace('while (1)', 'while (fatal_loop())') + "`n"
}
$fixture = Get-Content (Join-Path $PSScriptRoot 'fatal.c') -Raw
$sourcePath = Join-Path $buildDirectory 'fatal.c'
$exePath = Join-Path $buildDirectory 'fatal.exe'
[IO.File]::WriteAllText($sourcePath, $fixture.Replace('/* HANDLERS */', $functions))
& $compilerCommand.Source -std=gnu99 -Wall -Wextra -I (Join-Path $PSScriptRoot 'mocks') -I (Join-Path $firmwareRoot 'Core/Inc') $sourcePath -o $exePath
if ($LASTEXITCODE -ne 0) { throw 'Fatal handler compilation failed.' }
& $exePath
if ($LASTEXITCODE -ne 0) { throw 'Fatal handler regression failed.' }
