param([Parameter(Mandatory=$true)][string]$SourceDirectory,[Parameter(Mandatory=$true)][string]$Sysroot,[Parameter(Mandatory=$true)][string]$VulkanHeaders,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Fresh output required'}
$source=[IO.Path]::GetFullPath((Join-Path $SourceDirectory 'hybris/vulkan'))
$make=Get-Content (Join-Path $source 'Makefile.am') -Raw
$match=[regex]::Match($make,'(?s)libVkLayer_hybris_compat_la_SOURCES\s*=\s*(.*?)\r?\nlibVkLayer_hybris_compat_la_CFLAGS')
if(!$match.Success){throw 'Pinned compat source list missing'}
$files=@(($match.Groups[1].Value -replace '\\',' ' -split '\s+') | Where-Object {$_ -like '*.c'})
if($files.Count -lt 30 -or $files.Count -gt 80 -or @($files | Where-Object {$_ -notmatch '^(layer|compat)/[a-z0-9_]+\.c$'}).Count){throw 'Unexpected source list'}
$target=Join-Path $Sysroot 'usr/aarch64-linux-gnu'
$clang=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe'
New-Item -ItemType Directory -Path $out | Out-Null
New-Item -ItemType Directory -Path "$out/licenses" | Out-Null
Copy-Item -LiteralPath (Join-Path $SourceDirectory 'LICENSE.Apache2') -Destination "$out/licenses/libhybris-Apache2.txt"
Copy-Item -LiteralPath (Join-Path $source 'compat/shaders/LICENSE.bcdec') -Destination "$out/licenses/bcdec-MIT.txt"
$objects=@()
foreach($relative in $files){
    $object=Join-Path $out ($relative.Replace('/','_')+'.o')
    & $clang --target=aarch64-linux-gnu "--sysroot=$target" -std=gnu11 -O2 -fPIC -pthread -fvisibility=hidden -I "$VulkanHeaders/include" -c (Join-Path $source $relative) -o $object
    if($LASTEXITCODE){throw "Compile failed: $relative"}
    $objects+=$object
}
& $clang --target=aarch64-linux-gnu "--sysroot=$target" -fuse-ld=lld -nostdlib -shared "$target/lib/crti.o" @objects "$target/lib/libc.so.6" "$target/lib/libdl.so.2" "$target/lib/crtn.o" '-Wl,-z,defs' '-Wl,-z,nodelete' '-Wl,-soname,libVkLayer_hybris_compat.so' -o "$out/libVkLayer_hybris_compat.so"
if($LASTEXITCODE){throw 'Compat link failed'}
Get-FileHash -LiteralPath "$out/libVkLayer_hybris_compat.so"
