package org.mavk.probe;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import java.io.File;

public final class CoreComparisonActivity extends Activity {
    static { System.loadLibrary("mavk_probe"); }
    static native String runComparison(byte[] shader, String report, boolean validation);
    static native String runTexture(byte[] vertex, byte[] fragment, String report, boolean validation);
    static native String runTimeline(String report, boolean validation);
    static native String runBc(byte[] vertex,byte[] fragment,String report,boolean validation);
    static native String runBcChannels(byte[] vertex,byte[] fragment,String report,boolean validation);
    static native String runBc67(byte[] vertex,byte[] fragment,byte[] hdrFragment,String report,boolean validation);
    static native String runBcTexture(byte[] vertex,byte[] fragment,String report,boolean validation);
    static native String runNullBuffers(byte[] shader,String report,boolean validation);
    static native String runNullImages(byte[] shader,String report,boolean validation);
    static native String runNullAtomics(byte[] shader,String report,boolean validation);
    static native String runRuntimeNullAtomics(byte[] shader,String report,boolean validation,boolean multiSet);
    static native String runDescriptorGenerations(byte[] shader,String report,boolean validation);
    static native String runDescriptorTemplates(byte[] shader,String report,boolean validation);
    static native String runReuse(byte[] vertex,byte[] fragment,String report,boolean validation);
    static native String runRaster(byte[] vertex,byte[] fragment,String report,boolean validation);
    static native String runUi(byte[] vertex,byte[] fragment,String report,boolean validation);
    static native String runUiLayers(byte[] vertex,byte[] fragment,byte[] backgroundVertex,byte[] backgroundFragment,String report,boolean validation);
    static native String runUiBridge(byte[] vertex,byte[] fragment,byte[] backgroundVertex,byte[] backgroundFragment,byte[] compute,String report,boolean validation);
    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        TextView status = new TextView(this);
        status.setTextColor(0xffe5eeee); status.setBackgroundColor(0xff091016);
        status.setPadding(40,60,40,40); status.setTextSize(18);
        boolean texture = getIntent().getBooleanExtra("texture",false);
        status.setText(texture ? "Mavk Core textures\nVerifying mip upload and hardware sampling…\nNo VM or system changes." : "Mavk Core A/B\nDirect Vulkan vs Core on one hardware device.\nVerifying compute and image transfers…\nNo VM or system changes.");
        setContentView(status);
        new Thread(() -> {
            String result;
            try {
                if(getIntent().getBooleanExtra("runtimeGuards",false)||getIntent().getBooleanExtra("runtimeMultiSets",false)){
                    boolean multiSet=getIntent().getBooleanExtra("runtimeMultiSets",false);
                    try(java.io.InputStream shader=getAssets().open(multiSet?"null-atomics.multiset.original.spv":"null-atomics.original.spv")){
                        result=runRuntimeNullAtomics(shader.readAllBytes(),new File(getFilesDir(),"core-null-atomics.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false),multiSet);
                    }
                }else if(getIntent().getBooleanExtra("descriptorTemplates",false)){
                    try(java.io.InputStream shader=getAssets().open("descriptor-generation.comp.spv")){
                        result=runDescriptorTemplates(shader.readAllBytes(),new File(getFilesDir(),"core-descriptor-generations.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("descriptorGenerations",false)){
                    try(java.io.InputStream shader=getAssets().open("descriptor-generation.comp.spv")){
                        result=runDescriptorGenerations(shader.readAllBytes(),new File(getFilesDir(),"core-descriptor-generations.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("nullAtomics",false)){
                    try(java.io.InputStream shader=getAssets().open("null-atomics.comp.spv")){
                        result=runNullAtomics(shader.readAllBytes(),new File(getFilesDir(),"core-null-atomics.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("nullImages",false)){
                    try(java.io.InputStream shader=getAssets().open("null-images.comp.spv")){
                        result=runNullImages(shader.readAllBytes(),new File(getFilesDir(),"core-null-images.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("nullBuffers",false)){
                    try(java.io.InputStream shader=getAssets().open("null-buffers.comp.spv")){
                        result=runNullBuffers(shader.readAllBytes(),new File(getFilesDir(),"core-null-buffers.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("bcTexture",false)){
                    try(java.io.InputStream vertex=getAssets().open("sampled.vert.spv");java.io.InputStream fragment=getAssets().open("sampled.frag.spv")){
                        result=runBcTexture(vertex.readAllBytes(),fragment.readAllBytes(),new File(getFilesDir(),"core-bc-texture.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("bc67",false)){
                    try(java.io.InputStream vertex=getAssets().open("sampled.vert.spv");java.io.InputStream fragment=getAssets().open("sampled.frag.spv");java.io.InputStream hdr=getAssets().open("bc-hdr.frag.spv")){
                        result=runBc67(vertex.readAllBytes(),fragment.readAllBytes(),hdr.readAllBytes(),new File(getFilesDir(),"core-bc67.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("bcChannels",false)){
                    try(java.io.InputStream vertex=getAssets().open("sampled.vert.spv");java.io.InputStream fragment=getAssets().open("bc-channels.frag.spv")){
                        result=runBcChannels(vertex.readAllBytes(),fragment.readAllBytes(),new File(getFilesDir(),"core-bc-channels.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("bc",false)){
                    try(java.io.InputStream vertex=getAssets().open("sampled.vert.spv");java.io.InputStream fragment=getAssets().open("sampled.frag.spv")){
                        result=runBc(vertex.readAllBytes(),fragment.readAllBytes(),new File(getFilesDir(),"core-bc.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("bridgeLayers",false)){
                    try(java.io.InputStream v=getAssets().open("uioverlay.vert.spv");java.io.InputStream f=getAssets().open("uioverlay.frag.spv");java.io.InputStream bv=getAssets().open("vertex-buffer.vert.spv");java.io.InputStream bf=getAssets().open("offscreen.frag.spv");java.io.InputStream cs=getAssets().open("verify.comp.spv")){
                        result=runUiBridge(v.readAllBytes(),f.readAllBytes(),bv.readAllBytes(),bf.readAllBytes(),cs.readAllBytes(),new File(getFilesDir(),"core-ui-bridge.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("layers",false)){
                    try(java.io.InputStream v=getAssets().open("uioverlay.vert.spv");java.io.InputStream f=getAssets().open("uioverlay.frag.spv");java.io.InputStream bv=getAssets().open("vertex-buffer.vert.spv");java.io.InputStream bf=getAssets().open("offscreen.frag.spv")){
                        result=runUiLayers(v.readAllBytes(),f.readAllBytes(),bv.readAllBytes(),bf.readAllBytes(),new File(getFilesDir(),"core-ui-layers.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("ui",false)){
                    try(java.io.InputStream vertex=getAssets().open("uioverlay.vert.spv");java.io.InputStream fragment=getAssets().open("uioverlay.frag.spv")){
                        result=runUi(vertex.readAllBytes(),fragment.readAllBytes(),new File(getFilesDir(),"core-ui.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("raster",false)){
                    try(java.io.InputStream vertex=getAssets().open("sampled.vert.spv");java.io.InputStream fragment=getAssets().open("sampled.frag.spv")){
                        result=runRaster(vertex.readAllBytes(),fragment.readAllBytes(),new File(getFilesDir(),"core-raster.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("reuse",false)){
                    try(java.io.InputStream vertex=getAssets().open("sampled.vert.spv");java.io.InputStream fragment=getAssets().open("sampled.frag.spv")){
                        result=runReuse(vertex.readAllBytes(),fragment.readAllBytes(),new File(getFilesDir(),"core-reuse.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else if(getIntent().getBooleanExtra("timeline",false)){
                    result=runTimeline(new File(getFilesDir(),"core-timeline.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                }else if(texture){
                    try(java.io.InputStream vertex=getAssets().open("sampled.vert.spv");java.io.InputStream fragment=getAssets().open("sampled.frag.spv")){
                        result=runTexture(vertex.readAllBytes(),fragment.readAllBytes(),new File(getFilesDir(),"core-sampled.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                    }
                }else try(java.io.InputStream input=getAssets().open("verify.comp.spv")){
                    result = runComparison(input.readAllBytes(), new File(getFilesDir(), "core-comparison.json").getAbsolutePath(),getIntent().getBooleanExtra("validation",false));
                }
            } catch (Exception e) { result = "HARNESS_ERROR: " + e; }
            final String text = result;
            runOnUiThread(() -> { if (!isDestroyed()) status.setText(text); });
        }, "mavk-core-comparison").start();
    }
}
