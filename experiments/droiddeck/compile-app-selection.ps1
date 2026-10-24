param(
    [Parameter(Mandatory=$true)][string]$Work,
    [string]$Fork = 'E:/Mavk/work/DroidDeck-mavk',
    [string]$DependencyCache = 'E:/Mavk/cache/gradle/caches/modules-2/files-2.1'
)
$ErrorActionPreference = 'Stop'
$Work = (Resolve-Path -LiteralPath $Work).Path
function Artifact([string]$Group, [string]$Name, [string]$Version) {
    foreach ($cache in @($DependencyCache, 'C:/Users/denis/.gradle/caches/modules-2/files-2.1')) {
        $dir = Join-Path $cache "$Group/$Name/$Version"
        if (Test-Path -LiteralPath $dir) {
            $item = Get-ChildItem -LiteralPath $dir -Recurse -Filter "$Name-$Version.jar" | Select-Object -First 1
            if ($item) { return $item.FullName }
        }
    }
    throw "Missing artifact: $Group/$Name/$Version"
}
$compiler = @(
    (Artifact 'org.jetbrains.kotlin' 'kotlin-compiler-embeddable' '2.1.0'),
    (Artifact 'org.jetbrains.kotlin' 'kotlin-stdlib' '2.1.0'),
    (Artifact 'org.jetbrains.kotlin' 'kotlin-script-runtime' '2.1.0'),
    (Artifact 'org.jetbrains.kotlin' 'kotlin-reflect' '1.6.10'),
    (Artifact 'org.jetbrains.kotlin' 'kotlin-daemon-embeddable' '2.1.0'),
    (Artifact 'org.jetbrains.intellij.deps' 'trove4j' '1.0.20200330'),
    (Artifact 'org.jetbrains.kotlinx' 'kotlinx-coroutines-core-jvm' '1.6.4'),
    (Artifact 'org.jetbrains' 'annotations' '13.0')
)
$plugin = Artifact 'org.jetbrains.kotlin' 'kotlin-compose-compiler-plugin-embeddable' '2.1.0'
$own = "$Fork/app/build/tmp/kotlin-classes/debug"
$classpath = @(
    (Get-Content -LiteralPath "$Work/classpath.txt" -Raw).Trim(),
    'C:/Users/denis/AppData/Local/Android/Sdk/platforms/android-36/android.jar',
    $own,
    "$Fork/app/build/intermediates/javac/debug/compileDebugJavaWithJavac/classes",
    "$Fork/app/build/intermediates/compile_and_runtime_not_namespaced_r_class_jar/debug/processDebugResources/R.jar"
) -join ';'
$out = Join-Path $Work 'compiled'
if (Test-Path -LiteralPath $out) { throw "Preserve existing output: $out" }
$argsFile = Join-Path $Work 'compiler.args'
$arguments = @('-no-stdlib', '-no-reflect', '-jvm-target', '17', '-module-name', 'app_debug',
    ('-Xfriend-paths=' + $own), ('-Xplugin=' + $plugin), '-classpath', $classpath,
    '-d', $out, "$Work/session/MavkExperimental.kt", "$Work/SessionActivity.kt", "$Work/ui/FrontEndSetup.kt")
$quoted = $arguments | ForEach-Object { '"' + $_.Replace('\', '/').Replace('"', '\"') + '"' }
[System.IO.File]::WriteAllLines($argsFile, $quoted, [System.Text.UTF8Encoding]::new($false))
& 'C:/Program Files/Android/Android Studio/jbr/bin/java.exe' -Xmx2g -cp ($compiler -join ';') org.jetbrains.kotlin.cli.jvm.K2JVMCompiler "@$argsFile"
if ($LASTEXITCODE -ne 0) { throw "Kotlin compilation failed: $LASTEXITCODE" }
Write-Output "APP_SELECTION_KOTLIN_PASS output=$out"
