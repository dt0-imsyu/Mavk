package com.mavk.selectioncheck;

import android.app.Instrumentation;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import java.io.File;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.util.ArrayList;

/** Policy checks, optionally followed by a normal native session without Steam. */
public final class AhbSelectionCheck extends Instrumentation {
    private boolean normal;
    private String architecture = "aarch64";
    private String expectedLibraryHash = "99a12d70e88e14babcf27ad8dcb22804334a9e769bce2dcc89c7c5646096e663";
    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        normal = arguments != null && "1".equals(arguments.getString("normal"));
        if (arguments != null) architecture = arguments.getString("arch", "aarch64");
        if (arguments != null) expectedLibraryHash = arguments.getString("wsiSha256", expectedLibraryHash);
        require(expectedLibraryHash.matches("[0-9a-f]{64}"), "explicit WSI pin");
        require(architecture.equals("aarch64") || architecture.equals("x86_64"), "explicit test architecture");
        start();
    }
    static void require(boolean valid, String reason) { if (!valid) throw new AssertionError(reason); }
    static void restore(SharedPreferences prefs, String key, boolean existed, boolean value) {
        var edit = prefs.edit();
        if (existed) edit.putBoolean(key, value); else edit.remove(key);
        require(edit.commit(), "restore preference " + key);
    }
    @Override public void onStart() {
        Context context = getTargetContext();
        SharedPreferences prefs = context.getSharedPreferences("mavk", Context.MODE_PRIVATE);
        boolean hadEnabled = prefs.contains("enabled"), oldEnabled = prefs.getBoolean("enabled", false);
        boolean hadAhb = prefs.contains("ahb_display"), oldAhb = prefs.getBoolean("ahb_display", false);
        File directory = new File(context.getFilesDir(), "mavk-selection-check");
        Bundle result = new Bundle();
        int status = -1;
        try {
            require(!directory.exists() && directory.mkdir(), "isolated marker directory");
            ClassLoader loader = context.getClassLoader();
            Class<?> type = Class.forName("com.droiddeck.launcher.session.MavkExperimental", true, loader);
            Object policy = type.getField("INSTANCE").get(null);
            Method select = type.getMethod("select", Context.class, boolean.class);
            Method selectAhb = type.getMethod("selectAhbDisplay", Context.class, boolean.class);
            Method prepare = type.getMethod("prepareDisplay", Context.class, File.class, int.class, int.class, String.class);
            Method configure = type.getMethod("configure", Context.class, java.util.List.class);
            select.invoke(policy, context, true);
            selectAhb.invoke(policy, context, true);
            runOnMainSync(() -> {
                try { prepare.invoke(policy, context, directory, 1920, 1200, "steam"); }
                catch (Exception error) { throw new RuntimeException(error); }
            });
            File marker = new File(directory, "mavk-explicit-ahb.enable");
            require(marker.isFile() && Files.readString(marker.toPath()).equals("1920 1200\n"), "AHB prepared marker");
            ArrayList<String> env = new ArrayList<>();
            configure.invoke(policy, context, env);
            require(env.contains("DROIDDECK_MAVK_PRESENTATION=ahb-bgra"), "AHB environment");
            require(env.contains("DROIDDECK_MAVK_XCB_AHB_SHA256=" + expectedLibraryHash), "pinned candidate");
            require(env.stream().anyMatch(v -> v.startsWith("DROIDDECK_GAME_ENV_LAUNCHER=") && v.endsWith("/integration/droiddeck-game-env")), "bundled policy");
            selectAhb.invoke(policy, context, false);
            runOnMainSync(() -> {
                try { prepare.invoke(policy, context, directory, 1920, 1200, "steam"); }
                catch (Exception error) { throw new RuntimeException(error); }
            });
            require(!marker.exists(), "stale marker cleared");
            env.clear();
            configure.invoke(policy, context, env);
            require(env.contains("DROIDDECK_MAVK_PRESENTATION=x11-shm"), "explicit SHM reversal");
            require(env.stream().noneMatch(v -> v.startsWith("DROIDDECK_MAVK_XCB_AHB_SHA256=")), "no stale AHB pin");
            selectAhb.invoke(policy, context, true);
            runOnMainSync(() -> {
                try { prepare.invoke(policy, context, directory, 1921, 1200, "steam"); }
                catch (Exception error) { throw new RuntimeException(error); }
            });
            require(!marker.exists(), "invalid geometry falls back before compositor");
            env.clear();
            configure.invoke(policy, context, env);
            require(env.contains("DROIDDECK_MAVK_PRESENTATION=x11-shm"), "failed preparation stays SHM");
            if (normal) {
                File work = new File(context.getFilesDir(), "mavk-selection-normal");
                File done = new File(work, "done");
                require(new File(work, "run.sh").isFile() && !done.exists(), "fresh normal-session fixture");
                android.content.Intent intent = new android.content.Intent();
                intent.setClassName(context, "com.droiddeck.launcher.SessionActivity");
                intent.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK);
                intent.putExtra("mode", "run");
                intent.putExtra("program", new File(work, "run.sh").getPath());
                intent.putExtra("programArgs", new String[]{architecture});
                runOnMainSync(() -> context.startActivity(intent));
                long deadline = android.os.SystemClock.elapsedRealtime() + 90_000;
                while (!done.isFile() && android.os.SystemClock.elapsedRealtime() < deadline)
                    android.os.SystemClock.sleep(250);
                require(done.isFile(), "normal session did not complete in 90s");
                require(Files.readString(done.toPath()).trim().equals("0"), "normal-session native consumer failed");
                String log = Files.readString(new File(work, "result.log").toPath());
                require(log.contains("NORMAL_SESSION_AHB_READY") && log.contains("NORMAL_SESSION_D3D11_DONE") &&
                    log.contains("NORMAL_SESSION_POLICY_PASS helpers=2 ahb=1 shm=0 protonExecuted=0") &&
                    log.contains("MAVK_AHB_REQUEST flags=0") && log.contains("MAVK_WSI_AHB_IMPORT result=0") &&
                    log.contains("MAVK_LAYER_CORE_END liveZero=1") &&
                    log.contains("WINDOW_DEVICE_HRESULT=0x00000000") && log.contains("WINDOW_STREAM_END"), "normal-session AHB D3D11 receipts");
            }
            result.putString("stream", "APP_AHB_SELECTION_PASS realRuntime=1 ahb=1 shm=1 invalidGeometryFallback=1 steamStarted=0\n");
            if (normal) result.putString("stream", result.getString("stream") + "NORMAL_SESSION_D3D11_PASS steamStarted=0\n");
        } catch (Throwable error) {
            status = 0;
            result.putString("stream", "APP_AHB_SELECTION_FAIL " + android.util.Log.getStackTraceString(error));
        } finally {
            restore(prefs, "enabled", hadEnabled, oldEnabled);
            restore(prefs, "ahb_display", hadAhb, oldAhb);
            File marker = new File(directory, "mavk-explicit-ahb.enable");
            if (marker.exists()) marker.delete();
            directory.delete();
        }
        finish(status, result);
    }
}
