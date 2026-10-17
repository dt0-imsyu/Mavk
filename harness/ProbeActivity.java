package org.mavk.probe;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import java.io.File;

public final class ProbeActivity extends Activity {
    static { System.loadLibrary("mavk_probe"); }
    private static native String runProbe(byte[] shader, String report);
    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        TextView status = new TextView(this);
        status.setTextColor(0xffe5eeee);
        status.setBackgroundColor(0xff091016);
        status.setPadding(40,40,40,40);
        status.setTextSize(19);
        status.setText("Mavk M0\nNative Android Vulkan: inventory + verified compute.\nRunning bounded test…\nNo VM or system configuration changes.");
        setContentView(status);
        new Thread(() -> {
            String result;
            try (java.io.InputStream input = getAssets().open("verify.comp.spv")) {
                result = runProbe(input.readAllBytes(), new File(getFilesDir(), "native-compute.json").getAbsolutePath());
            } catch (Exception e) { result = "HARNESS_ERROR: " + e; }
            final String text = result;
            runOnUiThread(() -> { if (!isDestroyed()) status.setText(text); });
        }, "mavk-native-compute").start();
    }
}
