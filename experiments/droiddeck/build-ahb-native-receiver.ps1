param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $Output){throw 'Preserve existing native AHB receiver'}
$clang=Join-Path $env:LOCALAPPDATA 'Android/Sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe'
& $clang --target=aarch64-linux-android31 -std=c11 -O2 -Wall -Wextra -Werror -fPIE -pie "$PSScriptRoot/ahb-native-receiver.c" "$PSScriptRoot/../../adapters/android/ahb_image.c" -landroid -lvulkan -o $Output
if($LASTEXITCODE){throw 'Native AHB receiver build failed'}
Get-FileHash -LiteralPath $Output -Algorithm SHA256
