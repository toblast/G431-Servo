param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$cc = (Get-Command $Compiler -ErrorAction Stop).Source
$env:PATH = (Split-Path $cc) + ';' + $env:PATH
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = Join-Path $root 'build/tests/configuration'
New-Item -ItemType Directory -Force -Path $build | Out-Null
foreach ($optimization in @('O0', 'O2')) {
    $exe = Join-Path $build "configuration-$optimization.exe"
    & $cc -std=gnu99 "-$optimization" -Wall -Wextra -Werror -I (Join-Path $root 'tests/sensor/mocks') -I (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Inc') (Join-Path $PSScriptRoot 'configuration.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c') (Join-Path $root '01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw "Configuration regression compilation failed ($optimization)." }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Configuration regressions failed ($optimization)." }
}
