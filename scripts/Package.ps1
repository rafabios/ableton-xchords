param([ValidateSet('Debug','Release')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$artifacts=Join-Path $root "build/XChords_artefacts/$Configuration"
$dist=Join-Path $root 'dist'
$stage=Join-Path $dist ('stage-'+[guid]::NewGuid().ToString('N'))
$vst=Join-Path $artifacts 'VST3/Xchords - Harmony.vst3'
$exe=Join-Path $artifacts 'Standalone/Xchords - Harmony.exe'
if(!(Test-Path -LiteralPath $vst) -or !(Test-Path -LiteralPath $exe)){throw 'Build VST3 and standalone before packaging'}
New-Item -ItemType Directory -Force $dist,$stage | Out-Null
$packages=@()
foreach($kind in @('VST3','Standalone')) {
  $folder=Join-Path $stage $kind
  New-Item -ItemType Directory $folder | Out-Null
  $source=if($kind -eq 'VST3'){$vst}else{$exe}
  Copy-Item -LiteralPath $source -Destination $folder -Recurse
  Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $folder
  $juce=Join-Path $root 'build/_deps/juce-src/LICENSE.md'
  if(Test-Path -LiteralPath $juce){Copy-Item -LiteralPath $juce -Destination (Join-Path $folder 'JUCE-LICENSE.md')}
  $zip=Join-Path $dist "Xchords-Harmony-$kind-Windows-x64.zip"
  Compress-Archive -Path "$folder/*" -DestinationPath $zip -Force
  $packages+=$zip
}
$packages | ForEach-Object { $hash=Get-FileHash -LiteralPath $_ -Algorithm SHA256; "$($hash.Hash.ToLowerInvariant())  $([IO.Path]::GetFileName($_))" } | Set-Content -LiteralPath (Join-Path $dist 'SHA256SUMS.txt') -Encoding ascii
# Only remove the unique staging directory created by this invocation.
$resolved=[IO.Path]::GetFullPath($stage)
if(!$resolved.StartsWith([IO.Path]::GetFullPath($dist)+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Unexpected staging path'}
Remove-Item -LiteralPath $resolved -Recurse -Force
Get-ChildItem -LiteralPath $dist -File | Select-Object Name,Length
