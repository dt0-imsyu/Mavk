param([string]$OutputDirectory='E:/Mavk/artifacts/sarek-d3d11-shaders1-20261018',
      [string]$Compiler='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Fresh shader output required'}
if(!(Test-Path -LiteralPath $Compiler)){throw 'Windows SDK HLSL compiler required'}
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
& $Compiler /nologo /T vs_5_0 /E vsMain /Vn mavkFrameVS /Fh "$OutputDirectory/sarek-frame-vs.h" "$PSScriptRoot/sarek-frame.hlsl"
if($LASTEXITCODE){throw 'Vertex shader compilation failed'}
& $Compiler /nologo /T ps_5_0 /E psMain /Vn mavkFramePS /Fh "$OutputDirectory/sarek-frame-ps.h" "$PSScriptRoot/sarek-frame.hlsl"
if($LASTEXITCODE){throw 'Pixel shader compilation failed'}
& $Compiler /nologo /T ps_5_0 /E psTextured /Vn mavkTexturePS /Fh "$OutputDirectory/sarek-texture-ps.h" "$PSScriptRoot/sarek-frame.hlsl"
if($LASTEXITCODE){throw 'Texture shader compilation failed'}
& $Compiler /nologo /T ps_5_0 /E psFiltered /Vn mavkFilteredPS /Fh "$OutputDirectory/sarek-filtered-ps.h" "$PSScriptRoot/sarek-frame.hlsl"
if($LASTEXITCODE){throw 'Filtered shader compilation failed'}
Get-FileHash "$OutputDirectory/sarek-frame-vs.h","$OutputDirectory/sarek-frame-ps.h","$OutputDirectory/sarek-texture-ps.h","$OutputDirectory/sarek-filtered-ps.h"
