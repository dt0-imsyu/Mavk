param([Parameter(Mandatory=$true)][string]$Sysroot,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $Output){throw 'Choose fresh output'}
$clang=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe'
$target=Join-Path $Sysroot 'usr/aarch64-linux-gnu'
& $clang --target=aarch64-linux-gnu "--sysroot=$target" -fuse-ld=lld -std=c11 -O2 -g -Wall -Wextra -Werror -nostdlib -fPIE -pie "$target/lib/Scrt1.o" "$target/lib/crti.o" "$PSScriptRoot/hybris_probe.c" "$target/lib/libc.so.6" "$target/lib/libdl.so.2" "$target/lib/crtn.o" '-Wl,--dynamic-linker=/lib/ld-linux-aarch64.so.1' -o $Output
if($LASTEXITCODE){throw 'glibc probe build failed'}
Get-FileHash -LiteralPath $Output
