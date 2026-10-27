param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$firmware = Join-Path $root '01-Firmware/B-G431B-ESC1-v3'
$build = Join-Path $root 'build/tests/app'
New-Item -ItemType Directory -Force -Path $build | Out-Null
foreach ($optimization in @('O0', 'O2')) {
    $exe = Join-Path $build "app-$optimization.exe"
    & $cc -std=gnu99 "-$optimization" -Wall -Wextra -Werror -I (Join-Path $PSScriptRoot 'stubs') -I (Join-Path $firmware 'Core/Inc') (Join-Path $PSScriptRoot 'app.c') (Join-Path $PSScriptRoot 'app_fixture.c') (Join-Path $firmware 'Core/Src/app.c') (Join-Path $firmware 'Core/Src/control_table.c') -o $exe -lm
    if ($LASTEXITCODE -ne 0) { throw "Application compilation failed ($optimization)." }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Application regressions failed ($optimization)." }
}
