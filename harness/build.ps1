param([Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$ValidationLibrary,[string]$DevelopmentBundle,[string]$UiSourceDirectory,[string]$NullBufferShader,[string]$NullImageShader,[string]$NullAtomicShader)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Preserve previous build; choose fresh output directory'}
$sdk=Join-Path $env:LOCALAPPDATA 'Android/Sdk'
$ndk=Join-Path $sdk 'ndk/28.2.13676358'
$bt=Join-Path $sdk 'build-tools/36.0.0'
$jar=Join-Path $sdk 'platforms/android-36/android.jar'
$java='C:/Program Files/Android/Android Studio/jbr/bin'
$env:JAVA_HOME=Split-Path -Parent $java
$env:Path="$java;$env:Path"
New-Item -ItemType Directory -Path $out,"$out/classes","$out/dex","$out/payload/assets","$out/payload/lib/arm64-v8a" | Out-Null
$codec="$PSScriptRoot/../third_party/bcdec"
if((Get-FileHash "$codec/bcdec.h").Hash -ne '134520764D96F70A27DB814173616C89AD85DB95FBD3328417C7A8C77E7CA189'){throw 'Pinned unmodified bcdec required'}
if((Get-FileHash "$codec/LICENSE").Hash -ne '2414C782587F6E96661CB0D19D752432DDECFCC1A221B4CB12BC2253A54DC5AC'){throw 'Original bcdec license required'}
Copy-Item "$codec/LICENSE" "$out/payload/assets/bcdec-LICENSE.txt"
$bridgeDefines=@()
if($DevelopmentBundle){
    $bundleBytes=[IO.File]::ReadAllBytes([IO.Path]::GetFullPath($DevelopmentBundle))
    if(!$bundleBytes.Length -or $bundleBytes.Length -gt 4194304){throw 'Bounded fixed development artifact required'}
    # Generated compiler input, not a runtime filesystem lookup.
    $words=($bundleBytes|ForEach-Object {$_.ToString()}) -join ','
    [IO.File]::WriteAllText("$out/development-bundle.inc",('{'+$words+'}'),[Text.Encoding]::ASCII)
    $bridgeDefines+='-DMAVK_DEVELOPMENT_BUNDLE=1'
}
if($ValidationLibrary){Copy-Item -LiteralPath $ValidationLibrary -Destination "$out/payload/lib/arm64-v8a/libVkLayer_khronos_validation.so"}
if($NullBufferShader){Copy-Item -LiteralPath $NullBufferShader -Destination "$out/payload/assets/null-buffers.comp.spv"}
if($NullImageShader){Copy-Item -LiteralPath $NullImageShader -Destination "$out/payload/assets/null-images.comp.spv"}
if($NullAtomicShader){Copy-Item -LiteralPath $NullAtomicShader -Destination "$out/payload/assets/null-atomics.comp.spv"}
function Checked([string]$name){if($LASTEXITCODE){throw "$name failed ($LASTEXITCODE)"}}
$nativeExtra=@();$nativeDefines=@()
if($UiSourceDirectory){
    $ui=[IO.Path]::GetFullPath($UiSourceDirectory)
    $pins=@{'imgui.cpp'='e6a9bc1afe701da70d2cf8561bcd256c15c66f98';'imgui_draw.cpp'='3afcac4e9019772bbeb5ce1ff1793f59c916fb9e';'imgui_widgets.cpp'='110c4f86466d846aad1cf907e00573dd262f6281'}
    foreach($source in $pins.Keys){
        $file=Join-Path "$ui/external/imgui" $source;$data=[IO.File]::ReadAllBytes($file)
        $prefix=[Text.Encoding]::ASCII.GetBytes("blob $($data.Length)"+[char]0);$blob=[byte[]]::new($prefix.Length+$data.Length);$prefix.CopyTo($blob,0);$data.CopyTo($blob,$prefix.Length)
        if([Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($blob)).ToLowerInvariant() -ne $pins[$source]){throw 'Unmodified pinned ImGui source required'}
        $object="$out/$source.o"
        # Upstream warning policy separate from Mavk's -Werror; source unmodified.
        & "$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe" --target=aarch64-linux-android29 -std=c++17 -O2 -fPIC -I "$ui/external/imgui" -c $file -o $object
        Checked "pinned $source";$nativeExtra+=$object
    }
    foreach($stage in @('vert','frag')){
        $shaderFile="$ui/shaders/glsl/base/uioverlay.$stage"
        $expected=if($stage -eq 'vert'){'AE0981CB14C30D3C2129559E87F885C4BE99A5079AA95D5805B8331CFFC98B83'}else{'1B47364CE003776B67B0FA94FE6B3C1E39000F25BCAC50B19FE91713FCA42EE2'}
        if((Get-FileHash -LiteralPath $shaderFile).Hash -ne $expected){throw 'Unmodified pinned UI shader required'}
        & "$ndk/shader-tools/windows-x86_64/glslc.exe" --target-env=vulkan1.0 $shaderFile -o "$out/payload/assets/uioverlay.$stage.spv"
        Checked "upstream UI $stage"
    }
    Copy-Item "$ui/LICENSE.md" "$out/payload/assets/upstream-vulkan-LICENSE.md"
    Copy-Item "$ui/external/imgui/LICENSE.txt" "$out/payload/assets/upstream-imgui-LICENSE.txt"
    $nativeDefines=@('-DMAVK_NATIVE_UI=1','-I',"$ui/external/imgui")
}
& "$ndk/shader-tools/windows-x86_64/glslc.exe" --target-env=vulkan1.1 "$PSScriptRoot/shaders/verify.comp" -o "$out/payload/assets/verify.comp.spv"
Checked 'shader'
foreach($shader in @('vertex-buffer.vert','matrix.vert','offscreen.frag','scene.vert','scene.frag')){
    & "$ndk/shader-tools/windows-x86_64/glslc.exe" --target-env=vulkan1.0 "$PSScriptRoot/../icd/test/$shader" -o "$out/payload/assets/$shader.spv"
    Checked "$shader geometry test"
}
foreach($shader in @('triangle.vert','triangle.frag')){
    & "$ndk/shader-tools/windows-x86_64/glslc.exe" --target-env=vulkan1.1 "$PSScriptRoot/shaders/$shader" -o "$out/payload/assets/$shader.spv"
    Checked $shader
    & "$ndk/shader-tools/windows-x86_64/glslc.exe" --target-env=vulkan1.1 -mfmt=c "$PSScriptRoot/shaders/$shader" -o "$out/$shader.inc"
    Checked "$shader embedded words"
}
foreach($shader in @('sampled.vert','sampled.frag','sampled-scene.vert','sampled-scene.frag','bc-channels.frag','bc-hdr.frag','descriptor-generation.comp')){
    & "$ndk/shader-tools/windows-x86_64/glslc.exe" --target-env=vulkan1.1 "$PSScriptRoot/shaders/$shader" -o "$out/payload/assets/$shader.spv"
    Checked "$shader sampled image test"
}
& "$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe" --target=aarch64-linux-android29 -std=c++17 -O2 -g -Wall -Wextra -Wno-missing-field-initializers -Werror -fPIC -I "$PSScriptRoot/../core/include" -c "$PSScriptRoot/../core/src/core.cpp" -o "$out/core.o"
Checked 'Core compilation'
& "$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-ar.exe" rcs "$out/libmavk_core.a" "$out/core.o"
Checked 'Core archive'
& "$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe" --target=aarch64-linux-android29 -std=c++17 -O2 -g -Wall -Wextra -Wno-missing-field-initializers -Werror -fPIC @bridgeDefines -I $out -I "$PSScriptRoot/../core/include" -I "$PSScriptRoot/../bridge/include" -c "$PSScriptRoot/../bridge/src/bridge.cpp" -o "$out/bridge.o"
Checked 'Bridge compilation'
& "$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe" --target=aarch64-linux-android29 -std=c++17 -O2 -g -Wall -Wextra -Wno-missing-field-initializers -Werror -fPIC -I "$PSScriptRoot/../core/include" -I $out -c "$PSScriptRoot/../bridge/src/graphics.cpp" -o "$out/bridge-graphics.o"
Checked 'Bridge graphics compilation'
& "$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-ar.exe" rcs "$out/libmavk_bridge.a" "$out/bridge.o" "$out/bridge-graphics.o"
Checked 'Bridge archive'
& "$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe" --target=aarch64-linux-android29 -std=c++17 -O2 -g -Wall -Wextra -Wno-missing-field-initializers -Werror -shared -fPIC -static-libstdc++ '-Wl,-z,max-page-size=16384' @nativeDefines -I "$PSScriptRoot/../core/include" -I "$PSScriptRoot/../bridge/include" -I "$PSScriptRoot/../compatibility/include" "$PSScriptRoot/../compatibility/src/bc.cpp" "$PSScriptRoot/../compatibility/src/bc67.cpp" "$PSScriptRoot/../compatibility/src/texture.cpp" "$PSScriptRoot/native/vulkan_probe.cpp" "$PSScriptRoot/native/graphics_probe.cpp" "$PSScriptRoot/native/core_comparison.cpp" @nativeExtra "$out/libmavk_bridge.a" "$out/libmavk_core.a" -lvulkan -landroid -o "$out/payload/lib/arm64-v8a/libmavk_probe.so"
Checked 'native'
& "$bt/aapt2.exe" link -I $jar --manifest "$PSScriptRoot/AndroidManifest.xml" --min-sdk-version 29 --target-sdk-version 36 -o "$out/unsigned.apk"
Checked 'manifest'
& "$java/javac.exe" -source 17 -target 17 -classpath $jar -d "$out/classes" "$PSScriptRoot/ProbeActivity.java" "$PSScriptRoot/GraphicsActivity.java" "$PSScriptRoot/CoreComparisonActivity.java" "$PSScriptRoot/FeatureSuiteActivity.java" "$PSScriptRoot/BridgeActivity.java" "$PSScriptRoot/../adapters/android/org/mavk/bridge/BridgeNative.java" "$PSScriptRoot/../adapters/android/org/mavk/bridge/BridgeEndpoint.java"
Checked 'Java'
& "$java/jar.exe" cf "$out/classes.jar" -C "$out/classes" .
Checked 'Java archive'
& "$bt/d8.bat" --min-api 29 --lib $jar --output "$out/dex" "$out/classes.jar"
Checked 'dex'
& "$java/jar.exe" uf "$out/unsigned.apk" -C "$out/dex" classes.dex -C "$out/payload" assets -C "$out/payload" lib
Checked 'package'
& "$bt/zipalign.exe" -f -P 16 4 "$out/unsigned.apk" "$out/aligned.apk"
Checked 'alignment'
& "$bt/apksigner.bat" sign --ks "$env:USERPROFILE/.android/debug.keystore" --ks-pass pass:android --key-pass pass:android --out "$out/Mavk-native-probe.apk" "$out/aligned.apk"
Checked 'debug signing'
& "$bt/apksigner.bat" verify "$out/Mavk-native-probe.apk"
Checked 'signature verification'
Get-FileHash "$out/Mavk-native-probe.apk"
