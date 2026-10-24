$ErrorActionPreference = 'Stop'
$source = 'E:/Mavk/research/glmark2'
$out = 'E:/Mavk/artifacts/glmark2-20261023'
$sdk = 'C:/Users/denis/AppData/Local/Android/Sdk'
$build = "$sdk/build-tools/36.0.0"
$java = 'C:/Program Files/Android/Android Studio/jbr/bin'
$android = "$sdk/platforms/android-36/android.jar"
function Checked([string]$program, [string[]]$arguments) {
    & $program @arguments
    if ($LASTEXITCODE -ne 0) { throw "$program failed: $LASTEXITCODE" }
}
New-Item -ItemType Directory -Force "$out/java", "$out/classes", "$out/dex", "$out/stage/lib/arm64-v8a" | Out-Null
Checked "$build/aapt.exe" @('package','-f','-m','-S',"$source/android/res",'-J',"$out/java",'-M',"$source/android/AndroidManifest.xml",'-I',$android)
$sources = @(Get-ChildItem "$source/android/src", "$out/java" -Recurse -Filter '*.java' | ForEach-Object FullName)
Checked "$java/javac.exe" (@('-source','8','-target','8','-encoding','UTF-8','-classpath',$android,'-d',"$out/classes") + $sources)
$classes = @(Get-ChildItem "$out/classes" -Recurse -Filter '*.class' | ForEach-Object FullName)
Checked "$build/d8.bat" (@('--lib',$android,'--min-api','26','--output',"$out/dex") + $classes)
Copy-Item "$out/dex/classes.dex" "$out/stage/classes.dex"
Copy-Item "$out/android-libs/arm64-v8a/libglmark2-android.so" "$out/stage/lib/arm64-v8a/"
Checked "$build/aapt.exe" @('package','-f','-M',"$source/android/AndroidManifest.xml",'-S',"$source/android/res",'-A',"$source/data",'-I',$android,'-F',"$out/base.apk")
Checked "$java/jar.exe" @('uf',"$out/base.apk",'-C',"$out/stage",'.')
Checked "$build/zipalign.exe" @('-f','-p','4',"$out/base.apk","$out/aligned.apk")
if (-not (Test-Path "$out/benchmark-test.jks")) {
    Checked "$java/keytool.exe" @('-genkeypair','-keystore',"$out/benchmark-test.jks",'-storepass','android','-keypass','android','-alias','benchmark','-dname','CN=Mavk Benchmark Test','-keyalg','RSA','-validity','3650')
}
Checked "$build/apksigner.bat" @('sign','--ks',"$out/benchmark-test.jks",'--ks-pass','pass:android','--key-pass','pass:android','--out',"$out/glmark2-native-test.apk","$out/aligned.apk")
Checked "$build/apksigner.bat" @('verify','--verbose',"$out/glmark2-native-test.apk")
