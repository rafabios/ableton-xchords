param([string]$JuceSource='', [ValidateSet('Debug','Release')][string]$Configuration='Debug')
$ErrorActionPreference='Stop'
$cmakeCommand=Get-Command cmake -ErrorAction SilentlyContinue
$cmakeExe=if($cmakeCommand){$cmakeCommand.Source}else{& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.CMake.Project -find 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' | Select-Object -First 1}
if(-not $cmakeExe){throw 'CMake / Visual Studio Build Tools não encontrado'}
$argsConfigure=@('-S',$PSScriptRoot,'-B',"$PSScriptRoot/build",'-G','Visual Studio 17 2022','-A','x64')
if($JuceSource){$argsConfigure+="-DFETCHCONTENT_SOURCE_DIR_JUCE=$JuceSource"}
& $cmakeExe @argsConfigure
if($LASTEXITCODE -ne 0){throw 'Falha ao configurar'}
& $cmakeExe --build "$PSScriptRoot/build" --config $Configuration --parallel 2
if($LASTEXITCODE -ne 0){throw 'Falha ao compilar'}
& (Join-Path (Split-Path $cmakeExe) 'ctest.exe') --test-dir "$PSScriptRoot/build" -C $Configuration --output-on-failure
if($LASTEXITCODE -ne 0){throw 'Falha nos testes'}
