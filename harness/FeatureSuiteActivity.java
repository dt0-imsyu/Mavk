package org.mavk.probe;

import android.app.Activity;
import android.os.Bundle;
import android.widget.*;
import java.io.*;
import java.nio.file.Files;
import java.security.MessageDigest;
import org.json.*;

/** Thin Core gate: existing native tests, no VM, new renderer or test server. */
public final class FeatureSuiteActivity extends Activity {
    private TextView text;
    private Button run;
    private boolean busy;
    private static final String[] NAMES={"core-comparison.json","core-sampled.json","core-timeline.json","core-reuse.json","core-raster.json"};
    private static String hash(byte[] b)throws Exception {
        StringBuilder s=new StringBuilder();for(byte v:MessageDigest.getInstance("SHA-256").digest(b))s.append(String.format(java.util.Locale.ROOT,"%02x",v&255));return s.toString();
    }
    private JSONObject source(String name,String result)throws Exception {
        File f=new File(getFilesDir(),name);JSONObject s=new JSONObject().put("file",name);
        if(!f.isFile())return s.put("status","NOT_RUN");
        byte[] raw=Files.readAllBytes(f.toPath());JSONObject r=new JSONObject(new String(raw,java.nio.charset.StandardCharsets.UTF_8));
        boolean pass=result.equals(r.optString("result"))&&r.optBoolean("core_live_zero")&&r.optBoolean("validation_layers_enabled")&&r.optBoolean("synchronization_validation_requested")&&r.optInt("validation_errors",-1)==0&&r.optInt("validation_warnings",-1)==0;
        if(name.equals(NAMES[0]))pass&=r.optLong("verified_words")>0&&r.optLong("verified_image_pixels")>0;
        else if(name.equals(NAMES[1]))pass&=r.optInt("pixels_verified")==12288&&r.optInt("gpu_submissions")==12&&r.optJSONArray("samples")!=null&&r.getJSONArray("samples").length()==12;
        else if(name.equals(NAMES[2]))pass&=r.optBoolean("native_timeline_enabled")&&r.optInt("verified_words")==8192&&r.optInt("gpu_submissions")==4&&r.optInt("negative_tests")==3&&r.optInt("final_counter")==5&&r.optInt("timeout_tests")==1&&r.optInt("host_waits_for_gpu")==2;
        else if(name.equals(NAMES[3]))pass&=r.optInt("verified_pixels")==32768&&r.optInt("frames_per_arm")==16&&r.optInt("gpu_submissions")==32&&r.optInt("negative_tests")==2&&r.optInt("distinct_lod_outputs")==4&&r.optInt("baseline_pipeline_creates")==16&&r.optInt("retained_pipeline_creates")==1&&r.optInt("retained_per_frame_allocations",-1)==0;
        else pass&=r.optInt("verified_pixels")==18432&&r.optInt("gpu_submissions")==18&&r.optInt("pipeline_creates")==6&&r.optInt("negative_tests")==30&&r.optInt("per_frame_gpu_allocations",-1)==0&&"[0,128,255]".equals(String.valueOf(r.optJSONArray("source_alpha_cases")))&&"[\"replace\",\"straight_alpha_source_over\"]".equals(String.valueOf(r.optJSONArray("blend_modes")))&&"[\"full\",\"partial\",\"full_reset\"]".equals(String.valueOf(r.optJSONArray("clip_phases")));
        return s.put("status",pass?"PASS":"FAIL").put("sha256",hash(raw)).put("modified_ms",f.lastModified()).put("report",r);
    }
    private void row(JSONArray list,String feature,String implementation,String status,String evidence,String scope)throws Exception {
        list.put(new JSONObject().put("feature",feature).put("implementation",implementation).put("status",status).put("evidence",evidence).put("scope",scope));
    }
    private String aggregate(boolean fresh)throws Exception {
        JSONObject base=source(NAMES[0],"CORE_COMPUTE_AND_IMAGE_AB_PASS"),sampled=source(NAMES[1],"NATIVE_CORE_SAMPLED_MIPS_PASS");
        JSONObject timeline=source(NAMES[2],"NATIVE_CORE_TIMELINE_PASS");
        JSONObject reuse=source(NAMES[3],"NATIVE_SHARED_GRAPHICS_REUSE_PASS");
        JSONObject raster=source(NAMES[4],"NATIVE_SHARED_RASTER_PASS");
        String a=base.getString("status"),b=sampled.getString("status"),t=timeline.getString("status");JSONArray rows=new JSONArray();
        row(rows,"buffers.host_upload_readback","NATIVE",a,NAMES[0],"Mapped buffers, flush/invalidate, verified compute output; tested sizes only");
        row(rows,"images.rgba_transfer","NATIVE",a,NAMES[0],"RGBA image transfer/readback and resource recreate; not all Vulkan formats");
        row(rows,"compute.storage_buffer_spirv","NATIVE",a,NAMES[0],"Existing bounded compute shader through Core and direct Vulkan");
        row(rows,"graphics.sampled_mips","NATIVE",b,NAMES[1],"RGBA mip upload and nearest/linear filtering; bounded offscreen workload");
        row(rows,"shaders.graphics_spirv","NATIVE",b,NAMES[1],"Actual vertex/fragment pipelines; no universal SPIR-V feature claim");
        row(rows,"descriptors.combined_sampler","NATIVE",b,NAMES[1],"Native combined image sampler used by sampled workload; not a UBO or bindless descriptor gate");
        row(rows,"descriptors.compute_storage_buffer","NATIVE",a,NAMES[0],"Existing Core compute storage-buffer descriptor; not arbitrary descriptor layouts");
        String both=a.equals("PASS")&&b.equals("PASS")?"PASS":a.equals("FAIL")||b.equals("FAIL")?"FAIL":"NOT_RUN";
        row(rows,"synchronization.fences_barriers_lifetime","NATIVE",both,"both Core reports","SyncVal and resource-zero gates; bounded workloads, not universal synchronization coverage");
        JSONObject ar=base.optJSONObject("report"),br=sampled.optJSONObject("report");
        JSONObject tr=timeline.optJSONObject("report"),rr=reuse.optJSONObject("report"),cr=raster.optJSONObject("report");
        boolean same=ar!=null&&br!=null&&tr!=null&&rr!=null&&cr!=null&&!ar.optString("device").isEmpty()&&ar.optString("device").equals(br.optString("device"))&&ar.optString("device").equals(tr.optString("device"))&&ar.optString("device").equals(rr.optString("device"))&&ar.optString("device").equals(cr.optString("device"));
        row(rows,"compatibility.same_device_correctness","NATIVE",both.equals("PASS")?(same?"PASS":"FAIL"):both,"both Core reports","Same actual native device and exact output gates; not CTS or game compatibility");
        row(rows,"windows.wsi_cpu_readback_blit","EMULATED","ADAPTER_GATE_SEPARATE","Windows ICD evidence in docs/STATE.md","Implemented adapter fallback, not native zero-copy; not retested by Core suite");
        row(rows,"core.timeline_semaphore_wrapper","NATIVE",t,NAMES[2],"Native Vulkan1.2 feature explicitly queried/enabled; ordered GPU submissions, host wait/signal and timeout; no CPU emulation");
        row(rows,"graphics.retained_resources","NATIVE",reuse.getString("status"),NAMES[3],"32x32 shared renderer, 32 verified frames, one retained pipeline and no per-frame GPU allocations; synchronous readback, not guest or visible FPS");
        row(rows,"graphics.straight_alpha_source_over","NATIVE",raster.getString("status"),NAMES[4],"RGBA8 straight-alpha source-over, alpha0/128/255 and replacement control; not arbitrary blend equations or guest API support");
        row(rows,"graphics.dynamic_scissor","NATIVE",raster.getString("status"),NAMES[4],"Per-frame partial/full/reset clipping and invalid-bound rejection, same retained pipeline; bounded native workload only");
        row(rows,"core.external_memory_import_wrapper","UNSUPPORTED","NOT_IMPLEMENTED","core.hpp","No supported Core import path; no stock AVF zero-copy claim");
        row(rows,"windows.wddm_dwm_acceleration","UNSUPPORTED","NOT_IMPLEMENTED","separate WDDM R&D","Vulkan ICD success does not accelerate the Windows compositor");
        JSONObject report=new JSONObject().put("schema",1).put("result",both.equals("PASS")&&same&&t.equals("PASS")&&reuse.getString("status").equals("PASS")&&raster.getString("status").equals("PASS")?"MAVK_CORE_FEATURE_SUITE_PASS":"MAVK_CORE_FEATURE_SUITE_INCOMPLETE").put("sources",new JSONArray().put(base).put(sampled).put(timeline).put(reuse).put(raster)).put("features",rows).put("scope","Implemented bounded Core slices only. NATIVE/EMULATED/UNSUPPORTED describes Mavk implementation, not all physical GPU capabilities. Saved evidence is not a fresh run or conformance proof.");
        report.put("evidence_mode",fresh?"FRESH_RUN":"CACHED_REPORTS");report.put("generated_ms",System.currentTimeMillis());
        Files.writeString(new File(getFilesDir(),"feature-suite.json").toPath(),report.toString());
        StringBuilder display=new StringBuilder("Mavk Feature Suite\n\n");
        for(int i=0;i<rows.length();i++){JSONObject r=rows.getJSONObject(i);display.append(r.getString("implementation")).append(" · ").append(r.getString("status")).append("\n").append(r.getString("feature")).append("\n\n");}
        return display.append(fresh?"Fresh Core run.\n":"Saved reports: not a fresh run.\n").append("Unsupported here means not implemented in Mavk, not missing on Mali.\nWindows adapters and WDDM have separate gates.").toString();
    }
    private void execute(boolean fresh){
        if(busy)return;busy=true;run.setEnabled(false);text.setText(fresh?"Running existing Core gates…":"Reading saved Core evidence…");
        new Thread(()->{String result;try{
            if(fresh){
                Files.deleteIfExists(new File(getFilesDir(),"feature-suite.json").toPath());
                // Remove only these derived reports so an early failure cannot reuse a stale PASS.
                for(String n:NAMES)Files.deleteIfExists(new File(getFilesDir(),n).toPath());
                try(InputStream s=getAssets().open("verify.comp.spv")){CoreComparisonActivity.runComparison(s.readAllBytes(),new File(getFilesDir(),NAMES[0]).getAbsolutePath(),true);}
                try(InputStream v=getAssets().open("sampled.vert.spv");InputStream f=getAssets().open("sampled.frag.spv")){CoreComparisonActivity.runTexture(v.readAllBytes(),f.readAllBytes(),new File(getFilesDir(),NAMES[1]).getAbsolutePath(),true);}
                CoreComparisonActivity.runTimeline(new File(getFilesDir(),NAMES[2]).getAbsolutePath(),true);
                try(InputStream v=getAssets().open("sampled.vert.spv");InputStream f=getAssets().open("sampled.frag.spv")){CoreComparisonActivity.runReuse(v.readAllBytes(),f.readAllBytes(),new File(getFilesDir(),NAMES[3]).getAbsolutePath(),true);}
                try(InputStream v=getAssets().open("sampled.vert.spv");InputStream f=getAssets().open("sampled.frag.spv")){CoreComparisonActivity.runRaster(v.readAllBytes(),f.readAllBytes(),new File(getFilesDir(),NAMES[4]).getAbsolutePath(),true);}
            }
            result=aggregate(fresh);
        }catch(Exception e){result="Feature Suite error: "+e;}
        final String shown=result;runOnUiThread(()->{busy=false;if(!isDestroyed()){text.setText(shown);run.setEnabled(true);}});
        },"mavk-feature-suite").start();
    }
    @Override public void onCreate(Bundle saved){super.onCreate(saved);LinearLayout panel=new LinearLayout(this);panel.setOrientation(1);panel.setPadding(32,32,32,24);panel.setBackgroundColor(0xff091016);run=new Button(this);run.setText("Run Core checks (no VM)");run.setOnClickListener(v->execute(true));panel.addView(run);Button cached=new Button(this);cached.setText("Read saved results");cached.setOnClickListener(v->execute(false));panel.addView(cached);ScrollView scroll=new ScrollView(this);text=new TextView(this);text.setTextColor(0xffe5eeee);text.setTextSize(16);scroll.addView(text);panel.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));setContentView(panel);execute(getIntent().getBooleanExtra("run",false));}
}
