# Real pinned Sarek consumer;all external inputs/artifacts stay on healthy E.
param([switch]$ExperimentalD3dBcAdmission)
$ErrorActionPreference='Stop'
$out='E:/Mavk/artifacts/sarek-native-source-20261018'
if(Test-Path -LiteralPath $out){throw 'Fresh source directory required'}
New-Item -ItemType Directory -Path $out,"$out/sdk" | Out-Null
Copy-Item -LiteralPath 'E:/Mavk/research/sarek-b6653094/DXVK-Sarek-b6653094a2b8f3b824ffcf8f50aaa3b3da41263b' -Destination "$out/source" -Recurse
$dependencies=@(
    @('include/native/directx','Joshua-Ashton/mingw-directx-headers','9df86f2341616ef1888ae59919feaa6d4fad693d'),
    @('include/spirv','KhronosGroup/SPIRV-Headers','04f10f650d514df88b76d25e83db360142c7b174'),
    @('include/vulkan','KhronosGroup/Vulkan-Headers','8864cdc896bbc2a9b6eb36b3218fc9ef57908d77'),
    @('subprojects/dxbc-spirv','doitsujin/dxbc-spirv','c5c1a5b9b61773d59592f628faa39be9add26282'),
    @('subprojects/libdisplay-info','doitsujin/libdisplay-info','275e6459c7ab1ddd4b125f28d0440716e4888078'),
    @('subprojects/dxbc-spirv/submodules/spirv_headers','KhronosGroup/SPIRV-Headers','c8ad050fcb29e42a2f57d9f59e97488f465c436d')
)
foreach($dependency in $dependencies){
    $destination=Join-Path "$out/source" $dependency[0]
    if((Test-Path -LiteralPath $destination) -and (Get-ChildItem -LiteralPath $destination -Force)){throw 'Submodule destination not empty'}
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    $archive=Join-Path $out ($dependency[2]+'.tar.gz')
    Invoke-WebRequest -Uri "https://codeload.github.com/$($dependency[1])/tar.gz/$($dependency[2])" -OutFile $archive -TimeoutSec 90
    tar -xf $archive --strip-components 1 -C $destination
    if($LASTEXITCODE){throw "Submodule extraction failed: $($dependency[0])"}
    Get-FileHash -LiteralPath $archive
}
tar -xf E:/Mavk/artifacts/droiddeck-linuxfs-r9.tar.zst -C "$out/sdk" ./usr/include/SDL3 ./usr/lib/pkgconfig/sdl3.pc ./usr/lib/libSDL3.so.0.4.16
if($LASTEXITCODE){throw 'DroidDeck SDL3 SDK extraction failed'}
# Copies of the exact shipped binary avoid Windows symlink privilege requirements.
Copy-Item "$out/sdk/usr/lib/libSDL3.so.0.4.16" "$out/sdk/usr/lib/libSDL3.so"
Copy-Item "$out/sdk/usr/lib/libSDL3.so.0.4.16" "$out/sdk/usr/lib/libSDL3.so.0"
Get-FileHash "$out/sdk/usr/lib/libSDL3.so.0.4.16"
if($ExperimentalD3dBcAdmission){
    git -C "$out/source" apply "$PSScriptRoot/sarek-bc-format-admission.patch"
    if($LASTEXITCODE){throw 'Scoped experimental Sarek BC admission patch failed'}
}
