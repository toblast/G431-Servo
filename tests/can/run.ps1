param([string]$Compiler = $env:CC, [ValidateSet('all', 'strict')][string]$Suite = 'all')
$ErrorActionPreference = 'Stop'
$testRoot = $PSScriptRoot
$repoRoot = [IO.Path]::GetFullPath((Join-Path $testRoot '../..'))
$firmwareRoot = Join-Path $repoRoot '01-Firmware/B-G431B-ESC1-v3'
$buildRoot = Join-Path $testRoot 'build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

if (!$Compiler) {
    foreach ($candidate in @('clang', 'gcc')) {
        $found = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($found) { $Compiler = $found.Source; break }
    }
}
if (!$Compiler) {
    throw 'A native C compiler is required. Run ./tests/can/run.ps1 -Compiler <absolute-path-to-clang-or-gcc>.'
}
$compilerCommand = Get-Command $Compiler -ErrorAction Stop
$Compiler = $compilerCommand.Source
# GCC subprocesses and the produced executable may depend on adjacent DLLs.
$env:PATH = (Split-Path -Parent $Compiler) + [IO.Path]::PathSeparator + $env:PATH

$source = [IO.File]::ReadAllText((Join-Path $firmwareRoot 'Core/Src/main.c'))
$globals = [regex]::Match($source, '(?s)// CAN transport buffers.*?(?=/\* USER CODE END PV \*/)')
$settings = [regex]::Matches($source, '(?m)^#define (?:CAN_COMMAND_TIMEOUT_MS|FOC_TIMEOUT_MS)\s+[^\r\n]+').Value -join "`n"
$callback = [regex]::Match($source, '(?s)// CAN IT on message receive.*?(?=/\* USER CODE END 0 \*/)')
if (!$globals.Success -or !$callback.Success) { throw 'Production CAN source markers were not found.' }
$generated = Join-Path $buildRoot 'can_callback_test.c'
$focSource = [IO.File]::ReadAllText((Join-Path $firmwareRoot 'Core/Src/foc.c'))
$watchdog = [regex]::Match($focSource, '(?s)bool API_FOC_Check_Watchdog\(void\)\r?\n\{.*?\r?\n\}').Value
if (!$watchdog) { throw 'Production FOC watchdog was not found.' }
[IO.File]::WriteAllText($generated, "#include `"can_fixture.h`"`n" + $settings + "`n" + $globals.Value + "`n" + $watchdog + "`n" + $callback.Value + "`n#include `"can_tests.inc`"`n")
$executable = Join-Path $buildRoot 'can_callback_test.exe'
$compilerArgs = @('-std=c11', '-Wall', '-Wextra', '-Werror', '-O0',
    '-I', (Join-Path $testRoot 'stubs'), '-I', $testRoot,
    '-I', (Join-Path $firmwareRoot 'Core/Inc'), $generated, (Join-Path $firmwareRoot 'Core/Src/drive.c'), (Join-Path $firmwareRoot 'Core/Src/control_table.c'), '-o', $executable)
& $Compiler @compilerArgs
if ($LASTEXITCODE -ne 0) { throw "Native C compilation failed ($LASTEXITCODE)." }
& $executable $Suite
exit $LASTEXITCODE
