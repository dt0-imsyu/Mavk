package org.mavk.probe;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;
import java.io.File;
import java.nio.file.Files;
import java.nio.charset.StandardCharsets;
import java.nio.file.StandardCopyOption;

public final class GraphicsActivity extends Activity implements SurfaceHolder.Callback {
    static {System.loadLibrary("mavk_probe");}
    private static native long createSession();
    private static native void stopSession(long handle);
    private static native void releaseSession(long handle);
    private static native String runGraphics(long handle, Surface surface, byte[] vertex, byte[] fragment, String report, boolean core, boolean validation);
    private long session;
    private boolean available, destroyed;
    private Thread worker;
    private int generation;
    private SurfaceView viewport;
    private TextView status;
    private FrameLayout content;
    private final Handler ui=new Handler(Looper.getMainLooper());
    private final org.json.JSONArray lifecycle=new org.json.JSONArray();
    private int starts,completions,recreations;
    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        exportReports();
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        status=new TextView(this);status.setTextColor(0xffffffff);status.setBackgroundColor(0x99091016);
        status.setTextSize(16);status.setPadding(16,12,16,12);
        status.setText("Mavk M0 — offscreen correctness + native Surface; not guest GPU or measured visible FPS");
        content=new FrameLayout(this);createViewport();
        content.addView(status,new FrameLayout.LayoutParams(-1,-2));setContentView(content);
    }
    private void createViewport(){viewport=new SurfaceView(this);viewport.getHolder().setFixedSize(recreations==1?1280:1920,recreations==1?800:1200);viewport.getHolder().addCallback(this);content.addView(viewport,0);}
    private void event(String kind,String report){
        try{org.json.JSONObject row=new org.json.JSONObject();row.put("event",kind);row.put("generation",generation);row.put("report",report);row.put("monotonic_ns",System.nanoTime());lifecycle.put(row);
            org.json.JSONObject state=new org.json.JSONObject();state.put("result",destroyed?"ACTIVITY_DESTROYED":completions>=3?"BOUNDED_CYCLES_COMPLETE":"RUNNING");state.put("starts",starts);state.put("completions",completions);state.put("recreations",recreations);state.put("events",lifecycle);Files.writeString(new File(getFilesDir(),"graphics-lifecycle.json").toPath(),state.toString());
        }catch(Exception e){android.util.Log.e("MavkProbe","Lifecycle report failed",e);}
    }
    @Override public synchronized void surfaceCreated(SurfaceHolder holder){available=true;generation++;event("surface_created","");}
    @Override public void surfaceChanged(SurfaceHolder holder,int format,int width,int height){
        available=true;
        if(android.os.Build.VERSION.SDK_INT>=30)holder.getSurface().setFrameRate(60,Surface.FRAME_RATE_COMPATIBILITY_DEFAULT);
        beginIfReady();
    }
    private synchronized void beginIfReady(){
        if(!available||destroyed||worker!=null)return;
        final long handle=createSession();session=handle;
        final int workerGeneration=generation;
        final Surface surface=viewport.getHolder().getSurface();
        final String name="graphics-"+System.nanoTime()+".json";
        starts++;event("native_start",name);
        worker=new Thread(()->{
            String result;
            try(java.io.InputStream v=getAssets().open("triangle.vert.spv");java.io.InputStream f=getAssets().open("triangle.frag.spv")){
                Files.write(new File(getFilesDir(),"graphics-latest.txt").toPath(),name.getBytes(StandardCharsets.UTF_8));
                result=runGraphics(handle,surface,v.readAllBytes(),f.readAllBytes(),new File(getFilesDir(),name).getAbsolutePath(),getIntent().getBooleanExtra("core",false),getIntent().getBooleanExtra("validation",false));
                exportReports();
            }catch(Exception error){result="HARNESS_ERROR: "+error;}
            synchronized(this){session=0;releaseSession(handle);worker=null;}
            final String text=result;
            runOnUiThread(()->{completions++;event("native_retired",name);if(!destroyed){status.setText(text);if(available&&generation!=workerGeneration)beginIfReady();}});
        },"mavk-native-graphics");worker.start();
        if(getIntent().getBooleanExtra("lifecycle",false)&&recreations<2){
            ui.postDelayed(()->{if(!destroyed&&worker!=null&&generation==workerGeneration){event("remove_active_surface",name);content.removeView(viewport);recreations++;createViewport();}},3000);
        }
    }
    @Override public synchronized void surfaceDestroyed(SurfaceHolder holder){available=false;event("surface_destroyed","");if(session!=0)stopSession(session);}
    @Override public synchronized void onDestroy(){destroyed=true;ui.removeCallbacksAndMessages(null);event("activity_destroyed","");if(session!=0)stopSession(session);super.onDestroy();}
    private void exportReports(){
        File external=getExternalFilesDir(null);
        if(external==null)return;
        File[] reports=getFilesDir().listFiles((dir,name)->name.matches("graphics-[0-9]+\\.json"));
        if(reports==null)return;
        for(File report:reports)if(report.length()<=1024*1024){
            try{Files.copy(report.toPath(),new File(external,report.getName()).toPath(),StandardCopyOption.REPLACE_EXISTING);}
            catch(java.io.IOException error){android.util.Log.e("MavkProbe","Own report export failed",error);}
        }
    }
}
