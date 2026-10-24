# Generated binary fixture from the real pinned Sarek build,not a new shader.
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Fresh shader evidence directory required'}
New-Item -ItemType Directory -Path $out | Out-Null
$header='E:/Mavk/artifacts/sarek-native-build1-20261018/src/d3d11/libdxvk_d3d11.so.0.30002.p/d3d11_video_blit_frag.h'
$text=Get-Content -LiteralPath $header -Raw
if(!$text.Contains('const uint32_t d3d11_video_blit_frag[]')){throw 'Unexpected Sarek shader header'}
$words=[regex]::Matches($text,'0x[0-9a-fA-F]+') | ForEach-Object {[Convert]::ToUInt32($_.Value.Substring(2),16)}
if($words.Count -lt 5 -or $words[0] -ne 0x07230203 -or $words[1] -ne 0x00010600){throw 'Expected pinned SPIR-V1.6 shader'}
$writer=[IO.BinaryWriter]::new([IO.File]::Open("$out/sarek-video-blit.frag.spv",[IO.FileMode]::CreateNew))
try{foreach($word in $words){$writer.Write([uint32]$word)}}finally{$writer.Dispose()}
Copy-Item -LiteralPath 'E:/Mavk/artifacts/sarek-native-source-20261018/source/LICENSE' -Destination "$out/Sarek-LICENSE.txt"
Get-FileHash -LiteralPath $header,"$out/sarek-video-blit.frag.spv"
